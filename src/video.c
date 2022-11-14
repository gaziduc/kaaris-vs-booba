#define __STDC_CONSTANT_MACROS

#include <stdint.h>
#include <inttypes.h>
#include <windows.h>
#include <stdio.h>

#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_audio.h>

#include "event.h"
#include "video.h"
#include "game.h"


#define SDL_AUDIO_BUFFER_SIZE 1024;

typedef struct _PacketQueue
{
	AVPacketList *first, *last;
	int nb_packets, size;
	CRITICAL_SECTION cs;
	CONDITION_VARIABLE cv;

} PacketQueue;

int quit = 0;

PacketQueue audioq;

SwrContext *swrCtx;

void InitPacketQueue(PacketQueue * pq)
{
	memset(pq, 0, sizeof(PacketQueue));
	InitializeCriticalSection(&pq->cs);
	InitializeConditionVariable(&pq->cv);
}

int PacketQueuePut(PacketQueue * pq, const AVPacket * srcPkt)
{
	AVPacketList *elt;
	AVPacket pkt;
	int rv;
	if (!pq) return -1;
	rv = av_packet_ref(&pkt, srcPkt);
	if (rv) return rv;
	elt = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	if (!elt) return -1;
	elt->pkt = pkt;
	elt->next = NULL;

	EnterCriticalSection(&pq->cs);

	if (!pq->last)
		pq->first = elt;
	else
		pq->last->next = elt;
	pq->last = elt;
	pq->nb_packets++;
	pq->size += elt->pkt.size;
	WakeConditionVariable(&pq->cv);

	LeaveCriticalSection(&pq->cs);
	return 0;
}

int PacketQueueGet(PacketQueue *pq, AVPacket *pkt, int block)
{
	AVPacketList * elt;
	int rv;

	if (!pq || !pkt) return -1;

	EnterCriticalSection(&pq->cs);
	for (;;)
	{
		if (quit)
		{
			rv = -1;
			break;
		}

		elt = pq->first;
		if (elt)
		{
			pq->first = elt->next;
			if (!pq->first)
				pq->last = NULL;
			pq->nb_packets--;
			pq->size -= elt->pkt.size;
			*pkt = elt->pkt;
			av_free(elt);
			rv = 1;
			break;
		}
		else if (!block)
		{
			rv = 0;
			break;
		}
		else
		{
			SleepConditionVariableCS(&pq->cv, &pq->cs, INFINITE);
		}
	}
	LeaveCriticalSection(&pq->cs);
	return rv;
}

int DecodeAudioFrame(AVCodecContext *ctx, uint8_t *audioBuf, int bufSize)
{
	static AVPacket packet = { 0 };
	static uint8_t* packetData = NULL;
	static int packetSize = 0;
	static AVFrame * frame = NULL;
	static uint8_t converted_data[(192000 * 3) / 2];
	static uint8_t * converted;

	int len1, len2, dataSize = 0;

	if (!frame)
	{
		frame = av_frame_alloc();
		converted = &converted_data[0];
	}

	for (;;)
	{
		while (packetSize > 0) {
			int gotFrame = 0;
			int ret = avcodec_receive_frame(ctx, frame);
			if (ret == 0)
				gotFrame = 1;
			if (ret == AVERROR(EAGAIN))
				ret = 0;
			if (ret == 0)
				ret = avcodec_send_packet(ctx, &packet);
			if (ret == AVERROR(EAGAIN))
				ret = 0;
			else if (ret < 0)
				return 0;

			int len1 = packet.size;
			if (len1 < 0)
			{
				/* if error, skip frame */
				packetSize = 0;
				break;
			}

			packetData += len1;
			packetSize -= len1;
			if (gotFrame)
			{
				dataSize = av_samples_get_buffer_size(NULL, ctx->channels, frame->nb_samples, ctx->sample_fmt, 1);
				int outSize = av_samples_get_buffer_size(NULL, ctx->channels, frame->nb_samples, AV_SAMPLE_FMT_FLT, 1);
				len2 = swr_convert(swrCtx, &converted, frame->nb_samples, (const uint8_t**)&frame->data[0], frame->nb_samples);
				memcpy(audioBuf, converted_data, outSize);
				dataSize = outSize;
			}
			if (dataSize <= 0) {
				/* No data yet, get more frames */
				continue;
			}
			/* We have data, return it and come back for more later */
			return dataSize;
		}

		if (packet.data)
		{
			//av_free_packet(&packet);
		}

		if (quit)
			return -1;

		if (PacketQueueGet(&audioq, &packet, 1) < 0)
			return -1;

		packetData = packet.data;
		packetSize = packet.size;

	}

	return -1;
}

void AudioCallback(void *userdata, uint8_t * stream, int len)
{
	AVCodecContext * ctx = (AVCodecContext*)userdata;
	int len1, audioSize;

	static uint8_t audioBuf[(192000 * 3) / 2];
	static unsigned int audioBufSize = 0;
	static unsigned int audioBufIndex = 0;

	while (len > 0)
	{
		if (audioBufIndex >= audioBufSize)
		{
			// already sent all data; get more
			audioSize = DecodeAudioFrame(ctx, audioBuf, sizeof(audioBuf));
			if (audioSize < 0)
			{
				// error
				audioBufSize = SDL_AUDIO_BUFFER_SIZE;
				memset(audioBuf, 0, sizeof(audioBuf));
			}
			else
			{
				audioBufSize = audioSize;
			}
			audioBufIndex = 0;
		}
		len1 = audioBufSize - audioBufIndex;
		if (len1 > len)
			len1 = len;
		memcpy(stream, (uint8_t *)audioBuf + audioBufIndex, len1);
		len -= len1;
		stream += len1;
		audioBufIndex += len1;
	}
}













int playVideo(SDL_Window *window, SDL_Renderer *renderer, Input *in, Fonts *fonts, char *filename)
{
	int rv = 0;
	AVFormatContext *ictx = NULL, *octx = NULL;
	struct SwsContext *swsCtx = NULL;
	SDL_Texture *texture = NULL;
	SDL_AudioSpec wantedSpec = {0}, audioSpec = {0};
	char err[1024];
	unsigned long time1 = 0, time2 = 0;
	SDL_Color white = {255, 255, 255};
	SDL_Rect pos_dst;
    SDL_Texture *texte = RenderTextBlended(renderer, fonts->ocraext_message, "Appuyez sur ENTREE pour passer...", white);
    SDL_SetTextureBlendMode(texte, SDL_BLENDMODE_BLEND);
    int alpha = 0;
    int alphaGoingUp = 1;

    SDL_QueryTexture(texte, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.x = WINDOW_W / 2 - pos_dst.w / 2;
    pos_dst.y = WINDOW_H - pos_dst.h - 20;

    avdevice_register_all();
	avformat_network_init();

	rv = avformat_open_input(&ictx, filename, NULL, NULL);
	if(rv)
        goto cleanup;

	rv = avformat_find_stream_info(ictx, NULL);
	if(rv)
        goto cleanup;

	int audioStream = -1, videoStream = -1;

	for(unsigned int s = 0; s < ictx->nb_streams; ++s)
	{
		av_dump_format(ictx, s, filename, FALSE);
		if(ictx->streams[s]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStream < 0)
			audioStream = s;
		else if(ictx->streams[s]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStream < 0)
			videoStream = s;
	}

	if(audioStream < 0 && videoStream < 0)
        goto cleanup;

	AVCodec *audioCodec = NULL, *videoCodec = NULL;
	AVCodecContext *audioCtx = NULL, *videoCtx = NULL;

	if(audioStream >= 0)
	{
		audioCodec = avcodec_find_decoder(ictx->streams[audioStream]->codecpar->codec_id);
		if (!audioCodec)
			goto cleanup;
		audioCtx = avcodec_alloc_context3(audioCodec);
		if(!audioCtx)
            goto cleanup;
		rv = avcodec_parameters_to_context(audioCtx, ictx->streams[audioStream]->codecpar);
		if(rv)
            goto cleanup;
		rv = avcodec_open2(audioCtx, audioCodec, NULL);
		if(rv)
            goto cleanup;
		swrCtx = swr_alloc();
		if(!swrCtx)
            goto cleanup;

		av_opt_set_channel_layout(swrCtx, "in_channel_layout", audioCtx->channel_layout, 0);
		av_opt_set_channel_layout(swrCtx, "out_channel_layout", audioCtx->channel_layout, 0);
		av_opt_set_int(swrCtx, "in_sample_rate", audioCtx->sample_rate, 0);
		av_opt_set_int(swrCtx, "out_sample_rate", audioCtx->sample_rate, 0);
		av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", audioCtx->sample_fmt, 0);
		av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

		rv = swr_init(swrCtx);
		if(rv)
            goto cleanup;
	}

	if(videoStream >= 0)
	{
		videoCodec = avcodec_find_decoder(ictx->streams[videoStream]->codecpar->codec_id);
		if (!videoCodec)
			goto cleanup;
		videoCtx = avcodec_alloc_context3(videoCodec);
		if(!videoCtx)
            goto cleanup;
		rv = avcodec_parameters_to_context(videoCtx, ictx->streams[videoStream]->codecpar);
		if(rv)
            goto cleanup;
		rv = avcodec_open2(videoCtx, videoCodec, NULL);
		if(rv)
            goto cleanup;
		swsCtx = sws_getContext(videoCtx->width, videoCtx->height, videoCtx->pix_fmt, videoCtx->width, videoCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
		if(!swsCtx)
            goto cleanup;
	}

	SDL_AudioDeviceID devId = 0;

	if (audioCodec)
	{
		InitPacketQueue(&audioq);

		wantedSpec.channels = audioCtx->channels;
		wantedSpec.freq = audioCtx->sample_rate;
		wantedSpec.format = AUDIO_F32;
		wantedSpec.silence = 0;
		wantedSpec.samples = SDL_AUDIO_BUFFER_SIZE;
		wantedSpec.userdata = audioCtx;
		wantedSpec.callback = AudioCallback;

		devId = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &audioSpec, SDL_AUDIO_ALLOW_ANY_CHANGE);
        if (devId == 0)
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_OpenAudio", SDL_GetError(), window);
			goto cleanup;
		}

		SDL_PauseAudioDevice(devId, 0);
	}

	AVFrame *pFrame = NULL, *pFrameRGB = NULL;
	uint8_t *frameBuffer = NULL;
	int numBytes, width, height;

	if (videoCodec)
	{
		width = ictx->streams[videoStream]->codecpar->width;
		height = ictx->streams[videoStream]->codecpar->height;
		pFrame = av_frame_alloc();
		if(!pFrame)
            goto cleanup;
		pFrameRGB = av_frame_alloc();
		if(!pFrameRGB)
            goto cleanup;
		numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 8);
		if(numBytes < 1)
		{
			rv = numBytes;
			goto cleanup;
		}
		frameBuffer = (uint8_t*) av_malloc(numBytes);
		if (!frameBuffer)
            goto cleanup;

		rv = av_image_fill_arrays(&pFrameRGB->data[0], &pFrameRGB->linesize[0], frameBuffer, AV_PIX_FMT_RGB24, width, height, 1);
		if (rv < 0)
            goto cleanup;

		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, videoCtx->width, videoCtx->height);
		if (!texture)
            goto cleanup;
	}

	AVPacket packet;
	int iVideo = 0;

	while(av_read_frame(ictx, &packet) >= 0)
	{
		if(packet.stream_index == videoStream)
		{
			rv = avcodec_send_packet(videoCtx, &packet);
			if(rv)
                goto cleanup;

			while(!avcodec_receive_frame(videoCtx, pFrame))
			{
				sws_scale(swsCtx, pFrame->data, pFrame->linesize, 0, videoCtx->height, pFrameRGB->data, pFrameRGB->linesize);
				SDL_UpdateTexture(texture, NULL, pFrameRGB->data[0], pFrameRGB->linesize[0]);
				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, NULL, NULL);

				if(alphaGoingUp)
				{
				    alpha += 5;
				    if(alpha >= 255)
                        alphaGoingUp = 0;
				}
				else
                {
                    alpha -= 5;
				    if(alpha <= 0)
                        alphaGoingUp = 1;
                }


				SDL_SetTextureAlphaMod(texte, alpha);
				SDL_RenderCopy(renderer, texte, NULL, &pos_dst);
				SDL_RenderPresent(renderer);

				waitGame(&time1, &time2, DELAY_VIDEO);

				++iVideo;
			}
		}
		else if (packet.stream_index == audioStream)
			PacketQueuePut(&audioq, &packet);

		av_packet_unref(&packet);

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
            quit = 1;
            goto cleanup;
        }
	}

    cleanup:
	WakeConditionVariable(&audioq.cv);
	quit = 1;
	if(rv)
	{
		av_strerror(rv, err, sizeof(err));
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), window);
	}
	if(frameBuffer)
		av_free(frameBuffer);
	if(pFrame)
		av_frame_free(&pFrame);
	if(pFrameRGB)
		av_frame_free(&pFrameRGB);
	if(videoCtx)
		avcodec_close(videoCtx);
	if(audioCtx)
		avcodec_close(audioCtx);
	if(swsCtx)
		sws_freeContext(swsCtx);
	if(swrCtx)
		swr_free(&swrCtx);
	if(texture)
		SDL_DestroyTexture(texture);
	if(ictx)
		avformat_close_input(&ictx);
	if(octx)
		avformat_free_context(octx);

	avformat_network_deinit();
    SDL_DestroyTexture(texture);
	if (devId)
		SDL_CloseAudioDevice(devId);

	return rv;
}
