/*
 * Lecture de la video d'intro (FFmpeg + SDL2).
 *
 * Pourquoi cette version ?
 *  - Le son grésillait. Causes corrigées :
 *      1. la video ouvrait un SECOND peripherique audio alors que SDL_mixer avait deja ouvert le sien
 *         (formats/frequences differents, deux flux concurrents) -> le son video passe maintenant par
 *         le peripherique de SDL_mixer (Mix_HookMusic) dans le format exact de celui-ci ;
 *      2. le callback audio pouvait BLOQUER (attente d'un paquet) et laissait alors SDL lire un tampon
 *         non rempli -> le callback lit maintenant un tampon circulaire et complete par du silence
 *         s'il est vide, il ne bloque jamais ;
 *      3. SDL_AUDIO_ALLOW_ANY_CHANGE + aucune conversion si le format obtenu differait du format voulu ;
 *      4. les paquets audio n'etaient lus qu'au rythme du decodage video (file presque vide, a-coups) ;
 *      5. la variable globale "quit" n'etait jamais remise a 0 : plus de son aux lectures suivantes ;
 *      6. seul data[0] etait converti, des paquets etaient renvoyes deux fois au decodeur, etc.
 *  - La video est maintenant synchronisee sur l'horloge audio (pas de derive, images sautees si retard).
 *  - Plus de dependance a windows.h : le code est portable.
 */

#ifndef __EMSCRIPTEN__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "event.h"
#include "video.h"
#include "game.h"

/* FFmpeg >= 5.1 : AVChannelLayout (ch_layout). Avant : channel_layout / channels. */
#if (LIBAVUTIL_VERSION_MAJOR > 57) || (LIBAVUTIL_VERSION_MAJOR == 57 && LIBAVUTIL_VERSION_MINOR >= 24)
    #define VIDEO_HAVE_CH_LAYOUT 1
#else
    #define VIDEO_HAVE_CH_LAYOUT 0
#endif

#define AUDIO_RING_SECONDS      2.0     /* capacite du tampon circulaire           */
#define AUDIO_PREFILL_SECONDS   0.12    /* quantite de son avant de demarrer       */
#define VIDEO_DROP_LATE_SECONDS 0.15    /* au-dela, on saute l'image               */


/* ------------------------------------------------------------------------- */
/*  Tampon circulaire audio (producteur : thread principal, consommateur :    */
/*  thread audio de SDL_mixer)                                               */
/* ------------------------------------------------------------------------- */

typedef struct
{
    SDL_mutex *mutex;
    Uint8 *data;
    int size;
    int rpos, wpos, fill;
    int started;            /* tant que 0 : le callback joue du silence (pre-remplissage) */
    Uint8 silence;
    Uint64 consumed;        /* octets reellement joues */
} AudioRing;

static void SDLCALL audioHook(void *udata, Uint8 *stream, int len)
{
    AudioRing *ring = (AudioRing*) udata;
    int copied = 0;

    SDL_LockMutex(ring->mutex);

    if(ring->started && ring->fill > 0)
    {
        copied = (len < ring->fill) ? len : ring->fill;

        int first = ring->size - ring->rpos;
        if(first > copied)
            first = copied;

        memcpy(stream, ring->data + ring->rpos, first);
        memcpy(stream + first, ring->data, copied - first);

        ring->rpos = (ring->rpos + copied) % ring->size;
        ring->fill -= copied;
        ring->consumed += copied;
    }

    SDL_UnlockMutex(ring->mutex);

    if(copied < len) // sous-alimentation : silence, jamais de blocage ni de donnees perimees
        memset(stream + copied, ring->silence, len - copied);
}

static int ringFree(AudioRing *ring)
{
    int free_bytes;

    SDL_LockMutex(ring->mutex);
    free_bytes = ring->size - ring->fill;
    SDL_UnlockMutex(ring->mutex);

    return free_bytes;
}

static int ringFill(AudioRing *ring)
{
    int fill;

    SDL_LockMutex(ring->mutex);
    fill = ring->fill;
    SDL_UnlockMutex(ring->mutex);

    return fill;
}

static void ringWrite(AudioRing *ring, const Uint8 *src, int len)
{
    SDL_LockMutex(ring->mutex);

    int first = ring->size - ring->wpos;
    if(first > len)
        first = len;

    memcpy(ring->data + ring->wpos, src, first);
    memcpy(ring->data, src + first, len - first);

    ring->wpos = (ring->wpos + len) % ring->size;
    ring->fill += len;

    SDL_UnlockMutex(ring->mutex);
}


/* ------------------------------------------------------------------------- */
/*  Utilitaires                                                              */
/* ------------------------------------------------------------------------- */

static int sdlFormatToAV(SDL_AudioFormat format, enum AVSampleFormat *out)
{
    switch(format)
    {
        case AUDIO_U8:      *out = AV_SAMPLE_FMT_U8;  return 1;
        case AUDIO_S16SYS:  *out = AV_SAMPLE_FMT_S16; return 1;
        case AUDIO_S32SYS:  *out = AV_SAMPLE_FMT_S32; return 1;
        case AUDIO_F32SYS:  *out = AV_SAMPLE_FMT_FLT; return 1;
        default:            return 0;
    }
}

static int codecChannels(const AVCodecContext *ctx)
{
#if VIDEO_HAVE_CH_LAYOUT
    return ctx->ch_layout.nb_channels;
#else
    return ctx->channels;
#endif
}

/* Cree le convertisseur : format/frequence/canaux du decodeur -> ceux du peripherique SDL_mixer. */
static SwrContext* createResampler(AVCodecContext *ctx, enum AVSampleFormat out_fmt, int out_rate, int out_channels)
{
    SwrContext *swr = NULL;

#if VIDEO_HAVE_CH_LAYOUT
    AVChannelLayout in_layout, out_layout;

    if(av_channel_layout_copy(&in_layout, &ctx->ch_layout) < 0)
        return NULL;
    if(in_layout.order == AV_CHANNEL_ORDER_UNSPEC)
        av_channel_layout_default(&in_layout, in_layout.nb_channels);

    av_channel_layout_default(&out_layout, out_channels);

    if(swr_alloc_set_opts2(&swr, &out_layout, out_fmt, out_rate, &in_layout, ctx->sample_fmt, ctx->sample_rate, 0, NULL) < 0)
        swr = NULL;

    av_channel_layout_uninit(&in_layout);
    av_channel_layout_uninit(&out_layout);
#else
    int64_t in_layout = ctx->channel_layout;
    if(in_layout == 0)
        in_layout = av_get_default_channel_layout(ctx->channels);

    swr = swr_alloc_set_opts(NULL, av_get_default_channel_layout(out_channels), out_fmt, out_rate, in_layout, ctx->sample_fmt, ctx->sample_rate, 0, NULL);
#endif

    if(swr != NULL && swr_init(swr) < 0)
        swr_free(&swr);

    return swr;
}

static void showSkipHint(SDL_Renderer *renderer, SDL_Texture *texte, SDL_Rect *pos_dst, int *alpha, int *alphaGoingUp)
{
    if(texte == NULL)
        return;

    if(*alphaGoingUp)
    {
        *alpha += 5;
        if(*alpha >= 255)
        {
            *alpha = 255;
            *alphaGoingUp = 0;
        }
    }
    else
    {
        *alpha -= 5;
        if(*alpha <= 0)
        {
            *alpha = 0;
            *alphaGoingUp = 1;
        }
    }

    SDL_SetTextureAlphaMod(texte, *alpha);
    SDL_RenderCopy(renderer, texte, NULL, pos_dst);
}


/* ------------------------------------------------------------------------- */
/*  Lecture                                                                  */
/* ------------------------------------------------------------------------- */

typedef struct
{
    SDL_Renderer *renderer;
    Input *in;

    AVFormatContext *ictx;
    AVCodecContext *vctx, *actx;
    struct SwsContext *sws;
    SwrContext *swr;
    AVStream *vstream, *astream;

    SDL_Texture *texture;
    SDL_Texture *hint;
    SDL_Rect hint_pos;
    int alpha, alphaGoingUp;

    uint8_t *rgb_data[4];
    int rgb_linesize[4];

    AudioRing ring;
    int audio_on;
    int audio_rate, audio_channels, bytes_per_frame;
    enum AVSampleFormat audio_fmt;
    uint8_t *conv_buf;
    int conv_buf_samples;
    double audio_t0;            /* instant (s) du premier echantillon audio */
    int audio_t0_set;

    Uint64 wall_start;          /* horloge murale si pas d'audio */
    int wall_started;

    int shown_any;              /* au moins une image affichee */
    int skip;                   /* l'utilisateur a passe la video */
} VideoPlayer;

/* Lit les evenements. Renvoie 1 si la video doit s'arreter. */
static int pumpEvents(VideoPlayer *p)
{
    Input *in = p->in; // les macros KEY_* de event.h utilisent "in"

    updateEvents(in);

    if(in->quit)
        exit(EXIT_SUCCESS);

    if(KEY_ESCAPE || KEY_ENTER_MENU)
    {
        in->key[SDL_SCANCODE_SPACE] = 0;
        in->key[SDL_SCANCODE_RETURN] = 0;
        in->key[SDL_SCANCODE_KP_ENTER] = 0;
        in->controller->buttons[0] = 0;
        in->key[SDL_SCANCODE_ESCAPE] = 0;
        in->controller->buttons[6] = 0;
        p->skip = 1;
    }

    return p->skip;
}

/* Horloge maitre en secondes : l'audio effectivement joue, sinon le temps ecoule. */
static double masterClock(VideoPlayer *p)
{
    if(p->audio_on)
    {
        Uint64 consumed;

        SDL_LockMutex(p->ring.mutex);
        consumed = p->ring.consumed;
        SDL_UnlockMutex(p->ring.mutex);

        return p->audio_t0 + (double) consumed / ((double) p->audio_rate * p->bytes_per_frame);
    }

    if(!p->wall_started)
    {
        p->wall_start = SDL_GetPerformanceCounter();
        p->wall_started = 1;
    }

    return (double) (SDL_GetPerformanceCounter() - p->wall_start) / (double) SDL_GetPerformanceFrequency();
}

static double ringSeconds(VideoPlayer *p)
{
    return (double) ringFill(&p->ring) / ((double) p->audio_rate * p->bytes_per_frame);
}

static void startAudioIfReady(VideoPlayer *p, int force)
{
    if(!p->audio_on || p->ring.started)
        return;

    if(force || ringSeconds(p) >= AUDIO_PREFILL_SECONDS)
    {
        SDL_LockMutex(p->ring.mutex);
        p->ring.started = 1;
        SDL_UnlockMutex(p->ring.mutex);
    }
}

static void pushAudioFrame(VideoPlayer *p, AVFrame *frame)
{
    if(!p->audio_on)
        return;

    if(!p->audio_t0_set)
    {
        if(frame->pts != AV_NOPTS_VALUE)
            p->audio_t0 = frame->pts * av_q2d(p->astream->time_base);
        p->audio_t0_set = 1;
    }

    int out_samples = swr_get_out_samples(p->swr, frame->nb_samples);
    if(out_samples < 0)
        return;

    if(out_samples > p->conv_buf_samples)
    {
        av_freep(&p->conv_buf);
        p->conv_buf_samples = out_samples;
        p->conv_buf = av_malloc((size_t) out_samples * p->bytes_per_frame);
        if(p->conv_buf == NULL)
        {
            p->conv_buf_samples = 0;
            return;
        }
    }

    uint8_t *out[1] = { p->conv_buf };
    int converted = swr_convert(p->swr, out, out_samples, (const uint8_t**) frame->extended_data, frame->nb_samples);
    if(converted <= 0)
        return;

    int bytes = converted * p->bytes_per_frame;
    int offset = 0;

    while(offset < bytes && !p->skip)
    {
        int room = ringFree(&p->ring);

        if(room <= 0)
        {
            // tampon plein : on laisse le son avancer (et on garde l'ecran reactif)
            startAudioIfReady(p, 1);
            SDL_Delay(2);
            pumpEvents(p);
            continue;
        }

        int chunk = (bytes - offset < room) ? bytes - offset : room;
        ringWrite(&p->ring, p->conv_buf + offset, chunk);
        offset += chunk;

        startAudioIfReady(p, 0);
    }
}

static void showVideoFrame(VideoPlayer *p, AVFrame *frame)
{
    double t = 0;
    int have_t = 0;

    if(frame->best_effort_timestamp != AV_NOPTS_VALUE)
    {
        t = frame->best_effort_timestamp * av_q2d(p->vstream->time_base);
        have_t = 1;
    }

    if(p->audio_on && !p->ring.started)
    {
        // Pre-remplissage du son en cours : le son ne demarre que quand le tampon est assez plein, et
        // il ne se remplit qu'en lisant d'autres paquets. Attendre ici provoquerait un blocage :
        // on montre la premiere image, on saute les suivantes (elles sont dans les ~0,1 premieres secondes).
        if(p->shown_any)
            return;
    }
    else if(have_t)
    {
        // attend que l'horloge maitre atteigne l'instant de l'image
        Uint32 wait_start = SDL_GetTicks();

        for(;;)
        {
            double ahead = t - masterClock(p);

            if(ahead <= 0.002 || p->skip)
                break;

            // l'audio est sur le point de manquer : on n'attend plus, on retourne lire des paquets
            if(p->audio_on && ringSeconds(p) < 0.04)
                break;

            // garde-fou : l'horloge ne doit jamais pouvoir bloquer l'affichage
            if(SDL_GetTicks() - wait_start > 1000)
                break;

            if(pumpEvents(p))
                break;

            SDL_Delay(ahead > 0.02 ? 5 : 1);
        }

        if(p->audio_on && masterClock(p) - t > VIDEO_DROP_LATE_SECONDS)
            return; // trop en retard : image sautee pour rattraper le son
    }

    p->shown_any = 1;

    sws_scale(p->sws, (const uint8_t* const*) frame->data, frame->linesize, 0, p->vctx->height, p->rgb_data, p->rgb_linesize);
    SDL_UpdateTexture(p->texture, NULL, p->rgb_data[0], p->rgb_linesize[0]);

    SDL_RenderClear(p->renderer);
    SDL_RenderCopy(p->renderer, p->texture, NULL, NULL);
    showSkipHint(p->renderer, p->hint, &p->hint_pos, &p->alpha, &p->alphaGoingUp);
    SDL_RenderPresent(p->renderer);
}

static void drainVideo(VideoPlayer *p, AVFrame *frame)
{
    while(!p->skip && avcodec_receive_frame(p->vctx, frame) == 0)
    {
        showVideoFrame(p, frame);
        av_frame_unref(frame);
    }
}

static void drainAudio(VideoPlayer *p, AVFrame *frame)
{
    while(!p->skip && avcodec_receive_frame(p->actx, frame) == 0)
    {
        pushAudioFrame(p, frame);
        av_frame_unref(frame);
    }
}


int playVideo(SDL_Window *window, SDL_Renderer *renderer, Input *in, Fonts *fonts, char *filename)
{
    int rv = 0;
    VideoPlayer p;
    memset(&p, 0, sizeof(p));
    p.renderer = renderer;
    p.in = in;
    p.alphaGoingUp = 1;

    AVPacket *packet = NULL;
    AVFrame *vframe = NULL, *aframe = NULL;
    int vstream = -1, astream = -1;
    char err[AV_ERROR_MAX_STRING_SIZE] = "";
    SDL_Color white = {255, 255, 255, 255};

    p.hint = RenderTextBlended(renderer, fonts->ocraext_message, "Appuyez sur ENTREE pour passer...", white);
    if(p.hint != NULL)
    {
        SDL_SetTextureBlendMode(p.hint, SDL_BLENDMODE_BLEND);
        SDL_QueryTexture(p.hint, NULL, NULL, &p.hint_pos.w, &p.hint_pos.h);
        p.hint_pos.x = WINDOW_W / 2 - p.hint_pos.w / 2;
        p.hint_pos.y = WINDOW_H - p.hint_pos.h - 20;
    }

    rv = avformat_open_input(&p.ictx, filename, NULL, NULL);
    if(rv < 0)
    {
        p.ictx = NULL;
        goto cleanup;
    }

    rv = avformat_find_stream_info(p.ictx, NULL);
    if(rv < 0)
        goto cleanup;

    for(unsigned int s = 0; s < p.ictx->nb_streams; ++s)
    {
        enum AVMediaType type = p.ictx->streams[s]->codecpar->codec_type;

        if(type == AVMEDIA_TYPE_VIDEO && vstream < 0)
            vstream = s;
        else if(type == AVMEDIA_TYPE_AUDIO && astream < 0)
            astream = s;
    }

    if(vstream < 0)
    {
        rv = AVERROR_STREAM_NOT_FOUND;
        goto cleanup;
    }

    /* ---- Video ---- */
    {
        p.vstream = p.ictx->streams[vstream];
        const AVCodec *codec = avcodec_find_decoder(p.vstream->codecpar->codec_id);
        if(codec == NULL)
        {
            rv = AVERROR_DECODER_NOT_FOUND;
            goto cleanup;
        }

        p.vctx = avcodec_alloc_context3(codec);
        if(p.vctx == NULL)
        {
            rv = AVERROR(ENOMEM);
            goto cleanup;
        }

        rv = avcodec_parameters_to_context(p.vctx, p.vstream->codecpar);
        if(rv < 0)
            goto cleanup;

        rv = avcodec_open2(p.vctx, codec, NULL);
        if(rv < 0)
            goto cleanup;

        p.sws = sws_getContext(p.vctx->width, p.vctx->height, p.vctx->pix_fmt, p.vctx->width, p.vctx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
        if(p.sws == NULL)
        {
            rv = AVERROR(EINVAL);
            goto cleanup;
        }

        rv = av_image_alloc(p.rgb_data, p.rgb_linesize, p.vctx->width, p.vctx->height, AV_PIX_FMT_RGB24, 1);
        if(rv < 0)
            goto cleanup;

        p.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, p.vctx->width, p.vctx->height);
        if(p.texture == NULL)
        {
            rv = AVERROR(ENOMEM);
            goto cleanup;
        }
    }

    /* ---- Audio : via le peripherique deja ouvert par SDL_mixer ---- */
    if(astream >= 0)
    {
        int freq = 0, channels = 0;
        Uint16 format = 0;

        p.astream = p.ictx->streams[astream];
        const AVCodec *codec = avcodec_find_decoder(p.astream->codecpar->codec_id);

        if(codec != NULL && Mix_QuerySpec(&freq, &format, &channels) && sdlFormatToAV(format, &p.audio_fmt))
        {
            p.actx = avcodec_alloc_context3(codec);

            if(p.actx != NULL && avcodec_parameters_to_context(p.actx, p.astream->codecpar) >= 0 && avcodec_open2(p.actx, codec, NULL) >= 0 && codecChannels(p.actx) > 0)
            {
                p.swr = createResampler(p.actx, p.audio_fmt, freq, channels);

                if(p.swr != NULL)
                {
                    p.audio_rate = freq;
                    p.audio_channels = channels;
                    p.bytes_per_frame = channels * av_get_bytes_per_sample(p.audio_fmt);
                    p.ring.silence = (p.audio_fmt == AV_SAMPLE_FMT_U8) ? 0x80 : 0;
                    p.ring.size = (int) (AUDIO_RING_SECONDS * freq) * p.bytes_per_frame;
                    p.ring.data = av_malloc(p.ring.size);
                    p.ring.mutex = SDL_CreateMutex();

                    if(p.ring.data != NULL && p.ring.mutex != NULL)
                        p.audio_on = 1;
                }
            }
        }

        if(!p.audio_on) // pas de son plutot que du son corrompu : la video se joue quand meme
        {
            swr_free(&p.swr);
            avcodec_free_context(&p.actx);
            astream = -1;
        }
    }

    packet = av_packet_alloc();
    vframe = av_frame_alloc();
    aframe = av_frame_alloc();
    if(packet == NULL || vframe == NULL || aframe == NULL)
    {
        rv = AVERROR(ENOMEM);
        goto cleanup;
    }

    if(p.audio_on)
        Mix_HookMusic(audioHook, &p.ring);

    /* ---- Boucle de lecture ---- */
    while(!p.skip && av_read_frame(p.ictx, packet) >= 0)
    {
        if(packet->stream_index == vstream)
        {
            if(avcodec_send_packet(p.vctx, packet) >= 0)
                drainVideo(&p, vframe);
        }
        else if(packet->stream_index == astream && p.audio_on)
        {
            if(avcodec_send_packet(p.actx, packet) >= 0)
                drainAudio(&p, aframe);
        }

        av_packet_unref(packet);

        // ne pas prendre trop d'avance sur l'audio si le fichier est mal entrelace
        while(p.audio_on && !p.skip && ringSeconds(&p) > AUDIO_RING_SECONDS * 0.8)
        {
            startAudioIfReady(&p, 1);
            SDL_Delay(2);
            pumpEvents(&p);
        }

        pumpEvents(&p);
    }

    /* ---- Fin de fichier : on vide les decodeurs puis on laisse finir le son ---- */
    if(!p.skip)
    {
        avcodec_send_packet(p.vctx, NULL);
        drainVideo(&p, vframe);

        if(p.audio_on)
        {
            avcodec_send_packet(p.actx, NULL);
            drainAudio(&p, aframe);
            startAudioIfReady(&p, 1);

            while(!p.skip && ringFill(&p.ring) > 0)
            {
                SDL_Delay(5);
                pumpEvents(&p);
            }
        }
    }

    rv = 0;

cleanup:
    if(p.audio_on)
        Mix_HookMusic(NULL, NULL); // apres cet appel le thread audio n'utilise plus le tampon

    if(rv < 0)
    {
        av_strerror(rv, err, sizeof(err));
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur video", err, window);
    }

    av_frame_free(&vframe);
    av_frame_free(&aframe);
    av_packet_free(&packet);
    av_freep(&p.conv_buf);
    av_freep(&p.ring.data);
    if(p.ring.mutex != NULL)
        SDL_DestroyMutex(p.ring.mutex);
    if(p.rgb_data[0] != NULL)
        av_freep(&p.rgb_data[0]);
    if(p.sws != NULL)
        sws_freeContext(p.sws);
    swr_free(&p.swr);
    avcodec_free_context(&p.vctx);
    avcodec_free_context(&p.actx);
    if(p.ictx != NULL)
        avformat_close_input(&p.ictx);
    if(p.texture != NULL)
        SDL_DestroyTexture(p.texture);
    if(p.hint != NULL)
        SDL_DestroyTexture(p.hint);

    return rv;
}

#else /* __EMSCRIPTEN__ */

/*
 * Version web : FFmpeg n'est pas disponible, on laisse le navigateur decoder la video avec un element
 * HTML5 <video> place par-dessus le canvas. Le fichier est lu depuis le systeme de fichiers virtuel
 * Emscripten (--preload-file data) s'il y est, sinon charge par URL a cote de la page.
 *
 * Necessite -sASYNCIFY (ou equivalent) : la boucle d'attente cede la main au navigateur avec
 * emscripten_sleep().
 */

#include <emscripten.h>
#include <SDL2/SDL.h>
#include "event.h"
#include "video.h"
#include "game.h"

/* Renvoie 1 si la video a ete lancee. Etats : 0 en cours, 1 terminee, 2 erreur, 3 passee par clic/toucher */
EM_JS(int, web_video_start, (const char *path), {
    try {
        var p = UTF8ToString(path);
        var v = document.createElement('video');
        var src = p;

        try {
            var data = FS.readFile(p);
            var type = p.toLowerCase().endsWith('.webm') ? 'video/webm' : 'video/mp4';
            src = URL.createObjectURL(new Blob([data], {type: type}));
        } catch (e) { /* pas dans le FS virtuel : on tente l'URL directe */ }

        v.playsInline = true;
        v.setAttribute('playsinline', '');
        // l'element est cale sur le canvas du jeu (et suit ses changements de taille / plein ecran)
        var canvas = Module['canvas'] || document.getElementById('canvas');
        function place() {
            var r = canvas ? canvas.getBoundingClientRect() : { left: 0, top: 0, width: window.innerWidth, height: window.innerHeight };
            v.style.cssText = 'position:fixed;left:' + r.left + 'px;top:' + r.top + 'px;width:' + r.width + 'px;height:' + r.height +
                              'px;object-fit:contain;background:#000;z-index:2147483646;';
            hint.style.left = r.left + 'px'; hint.style.width = r.width + 'px';
            hint.style.top = (r.top + r.height - 44) + 'px'; hint.style.bottom = 'auto';
        }

        var hint = document.createElement('div');
        hint.textContent = 'Appuyez sur ENTREE pour passer...';
        hint.style.cssText = 'position:fixed;left:0;right:0;bottom:20px;text-align:center;color:#fff;font:20px monospace;' +
                             'z-index:2147483647;pointer-events:none;animation:kvbblink 1.6s ease-in-out infinite alternate;';
        hint.style.position = 'fixed';
        var style = document.createElement('style');
        style.textContent = '@keyframes kvbblink{from{opacity:0}to{opacity:1}}';

        var st = { el: v, hint: hint, style: style, state: 0, src: src, blob: (src !== p), place: place,
                   unmute: function () { v.muted = false; } };
        Module.kvbVideo = st;

        v.onended = function () { st.state = 1; };
        v.onerror = function () { st.state = 2; };
        v.onclick = function () { st.state = 3; };
        v.ontouchend = function () { st.state = 3; };
        v.src = src;

        document.head.appendChild(style);
        document.body.appendChild(v);
        document.body.appendChild(hint);
        place();
        window.addEventListener('resize', place);
        document.addEventListener('fullscreenchange', place);

        // Les navigateurs bloquent souvent l'autoplay avec son : on retente en muet, puis on remet le
        // son des la premiere interaction (touche ou clic).
        var pr = v.play();
        if (pr && pr.catch) {
            pr.catch(function () {
                v.muted = true;
                v.play().catch(function () { st.state = 2; });
                window.addEventListener('keydown', st.unmute, { once: true });
                window.addEventListener('pointerdown', st.unmute, { once: true });
            });
        }
        return 1;
    } catch (e) {
        console.error('Lecture video impossible :', e);
        return 0;
    }
});

EM_JS(int, web_video_state, (), {
    return Module.kvbVideo ? Module.kvbVideo.state : 2;
});

EM_JS(void, web_video_stop, (), {
    var st = Module.kvbVideo;
    if (!st) return;
    try { st.el.pause(); } catch (e) {}
    window.removeEventListener('resize', st.place);
    document.removeEventListener('fullscreenchange', st.place);
    window.removeEventListener('keydown', st.unmute);
    window.removeEventListener('pointerdown', st.unmute);
    st.el.remove();
    st.hint.remove();
    st.style.remove();
    if (st.blob) URL.revokeObjectURL(st.src);
    Module.kvbVideo = null;
});

int playVideo(SDL_Window *window, SDL_Renderer *renderer, Input *in, Fonts *fonts, char *filename)
{
    if(!web_video_start(filename))
        return -1;

    while(web_video_state() == 0)
    {
        updateEvents(in);

        if(in->quit)
            break;

        if(KEY_ESCAPE || KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller->buttons[0] = 0;
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller->buttons[6] = 0;
            break;
        }

        emscripten_sleep(16);
    }

    web_video_stop();

    return 0;
}

#endif // __EMSCRIPTEN__
