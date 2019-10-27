#include <stdio.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL2_framerate.h>
#include "event.h"
#include "game.h"
#include "key.h"
#include "transition.h"
#include "editor.h"
#include "option.h"
#include "file.h"
#include "multi.h"
#include "text.h"
#include "version.h"

enum {TITLE, SOLO, MULTIPLAYER, ONLINE, OPTIONS, QUIT, INFO, VERSION, NUM_TEXT};

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    Input *in = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Surface *icon = NULL;
    Mix_Music *music = NULL;
    FPSmanager *fps = NULL;
    int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    int mix_flags = MIX_INIT_MP3;
    int initted = 0, escape = 0;
    Fonts *fonts;
    Pictures *pictures;
    Sounds *sounds;
    Settings *settings;
    unsigned long frame_num = 0;
    SDL_Rect pos_dst[NUM_TEXT];
    SDL_Rect pos_fs;
    SDL_Texture *texture[NUM_TEXT];
    SDL_Color white = {255, 255, 255, 255};
    int selected = SOLO;
    char str[256] = "";
    pos_fs.x = 0;
    pos_fs.y = 0;
    pos_fs.w = WINDOW_W;
    pos_fs.h = WINDOW_H;


    srand(time(NULL));

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC) < 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2", SDL_GetError(), NULL);
        exit(EXIT_FAILURE);
    }

    window = SDL_CreateWindow(DESCRIPTION_STR, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    if(window == NULL)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not create window", SDL_GetError(), NULL);
        exit(EXIT_FAILURE);
    }

    SDL_ShowCursor(SDL_DISABLE);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if(renderer == NULL)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not create renderer", SDL_GetError(), window);
        exit(EXIT_FAILURE);
    }

    initted = IMG_Init(img_flags);
    if((initted & img_flags) != img_flags)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2_image", IMG_GetError(), window);
        exit(EXIT_FAILURE);
    }

    if(TTF_Init() == -1)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2_ttf", TTF_GetError(), window);
        exit(EXIT_FAILURE);
    }

    initted = Mix_Init(mix_flags);
    #ifdef __WIN64__
        if((initted & mix_flags) != mix_flags)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2_mixer", Mix_GetError(), window);
            exit(EXIT_FAILURE);
        }
    #endif // WIN64

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    Mix_AllocateChannels(32); // 32 to be sure that all sfx will be heard (multiple coins, ...)

    if(SDLNet_Init() == -1)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2_net", SDLNet_GetError(), window);
        exit(EXIT_FAILURE);
    }

    in = malloc(sizeof(Input));
    if(in == NULL)
        exit(EXIT_FAILURE);
    memset(in, 0, sizeof(*in));

    fonts = loadFonts();
    pictures = loadPictures(renderer);
    sounds = loadSounds();
    settings = loadSettings();

    Mix_VolumeMusic(settings->music_volume);
    setSfxVolume(sounds, settings->sfx_volume);

    icon = IMG_Load("data/gfx/life.bmp");
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);

    if(settings->fullscreen)
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    fps = malloc(sizeof(FPSmanager));
    if(fps == NULL)
        exit(EXIT_FAILURE);

    SDL_initFramerate(fps);
    SDL_setFramerate(fps, 30);

    intro(renderer, in, pictures, fonts, sounds, fps);

    SDL_setFramerate(fps, 60);

    texture[TITLE] = RenderTextBlended(renderer, fonts->preview_title, "Kaaris vs Booba", white);
    texture[SOLO] = RenderTextBlended(renderer, fonts->ocraext_message, "Solo", white);
    texture[MULTIPLAYER] = RenderTextBlended(renderer, fonts->ocraext_message, "Multi local", white);
    texture[ONLINE] = RenderTextBlended(renderer, fonts->ocraext_message, "Multi en LAN", white);
    texture[OPTIONS] = RenderTextBlended(renderer, fonts->ocraext_message, "Options", white);
    texture[QUIT] = RenderTextBlended(renderer, fonts->ocraext_message, "Quitter", white);

    sprintf(str, "David Gazi - Version %s (64 bits) - 2019", VERSION_STR);
    texture[VERSION] = RenderTextBlended(renderer, fonts->ocraext_version, str, white);
    texture[INFO] = RenderTextBlended(renderer, fonts->ocraext_version, "Editeur de niveau : F8", white);

    for(int i = TITLE; i < NUM_TEXT; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;

        if(i == TITLE)
            pos_dst[i].y = 80;
        else if(i < INFO)
            pos_dst[i].y = 255 + (i - 1) * 70;
        else if(i < VERSION)
            pos_dst[i].y = WINDOW_H - 70;
        else
            pos_dst[i].y = WINDOW_H - 48;

    }

    music = Mix_LoadMUS("data/musics/menu.mp3");
    Mix_PlayMusic(music, -1);

    transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 1, fps);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            escape = 1;
        if(KEY_UP_MENU)
        {
            in->key[SDL_SCANCODE_UP] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(selected > SOLO)
            {
                selected--;
                Mix_PlayChannel(-1, sounds->select, 0);
            }
        }
        if(KEY_DOWN_MENU)
        {
            in->key[SDL_SCANCODE_DOWN] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(selected < QUIT)
            {
                selected++;
                Mix_PlayChannel(-1, sounds->select, 0);
            }
        }
        if(KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;

            if(selected == QUIT)
                escape = 1;
            else
            {
                Mix_PlayChannel(-1, sounds->enter, 0);

                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 0, fps);

                if(selected == SOLO)
                    selectMode(renderer, pictures, fonts, in, sounds, &music, settings, 1, NULL, fps);
                else if(selected == MULTIPLAYER)
                    selectMultiCommandType(renderer, in, fonts, pictures, sounds, &music, settings, fps);
                else if(selected == ONLINE)
                    hostOrJoin(renderer, pictures, fonts, in, sounds, &music, settings, fps);
                else if(selected == OPTIONS)
                    displayOptions(renderer, window, pictures, fonts, sounds, settings, in, fps);

                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, EXITING, 1, fps);
            }
        }
        if(in->key[SDL_SCANCODE_F8])
        {
            in->key[SDL_SCANCODE_F8] = 0;
            editor(renderer, in, pictures, fonts, fps);
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, &pos_fs);

        for(int i = TITLE; i < NUM_TEXT; i++)
        {
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);
            if(selected == i)
            {
                SDL_Rect pos_arrow;
                SDL_QueryTexture(pictures->HUDlife, NULL, NULL, &pos_arrow.w, &pos_arrow.h);
                pos_arrow.x = pos_dst[i].x - pos_arrow.w - 40 + (frame_num % 60 < 30 ? frame_num % 30 : 30 - frame_num % 30);
                pos_arrow.y = pos_dst[i].y + pos_dst[i].h / 2 - pos_arrow.h / 2;
                SDL_RenderCopy(renderer, pictures->HUDlife, NULL, &pos_arrow);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        frame_num++;
    }

    return EXIT_SUCCESS;
}

