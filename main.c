#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "event.h"
#include "game.h"
#include "video.h"
#include "key.h"
#include "transition.h"
#include "editor.h"
#include "option.h"

enum {TITLE, PLAY_GAME, COMMANDS, OPTIONS, SCORES, QUIT, NUM_TEXT};

#define LOADING_W       WINDOW_W / 2
#define LOADING_H       WINDOW_H / 2


int main(int argc, char *argv[])
{
    Input *in = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Surface *icon = NULL;
    Mix_Music *music = NULL;
    int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    int mix_flags = MIX_INIT_MP3;
    int initted = 0, escape = 0;
    Fonts *fonts;
    Pictures *pictures;
    Sounds *sounds;
    Settings *settings;
    unsigned long time1 = 0, time2 = 0;
    SDL_Rect pos_dst[NUM_TEXT];
    SDL_Texture *texture[NUM_TEXT];
    SDL_Color white = {255, 255, 255};
    int selected = PLAY_GAME;


    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC) < 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2", SDL_GetError(), NULL);
        exit(EXIT_FAILURE);
    }


    window = SDL_CreateWindow("Kaaris vs Booba", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LOADING_W, LOADING_H, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    if(window == NULL)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not create window", SDL_GetError(), NULL);
        exit(EXIT_FAILURE);
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if(renderer == NULL)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not create renderer", SDL_GetError(), window);
        exit(EXIT_FAILURE);
    }


    SDL_RenderClear(renderer);
    SDL_Surface *loading_temp = SDL_LoadBMP("./data/background/loading.bmp");
    SDL_Texture *loading = SDL_CreateTextureFromSurface(renderer, loading_temp);
    SDL_FreeSurface(loading_temp);
    SDL_RenderCopy(renderer, loading, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(loading);


    srand(time(NULL));

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
    if((initted & mix_flags) != mix_flags)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2_mixer", Mix_GetError(), window);
        exit(EXIT_FAILURE);
    }

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    Mix_AllocateChannels(16); // 16 to be sure that all sfx will be heard (multiple coins, ...)

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

    Mix_Chunk *sound = Mix_LoadWAV("./data/sfx/oh_clique.wav");
    Mix_VolumeChunk(sound, settings->sfx_volume);
    Mix_PlayChannel(-1, sound, 0);

    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetWindowSize(window, WINDOW_W, WINDOW_H);
    SDL_SetWindowBordered(window, SDL_TRUE);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    icon = IMG_Load("./data/hud/life.bmp");
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);

    if(settings->fullscreen)
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    playVideo(renderer, in, fonts, "./data/video/intro.mp4");


    texture[TITLE] = RenderTextBlended(renderer, fonts->ocraext_title, "Kaaris vs Booba", white);
    texture[PLAY_GAME] = RenderTextBlended(renderer, fonts->ocraext_message, "Jouer", white);
    texture[COMMANDS] = RenderTextBlended(renderer, fonts->ocraext_message, "Commandes", white);
    texture[OPTIONS] = RenderTextBlended(renderer, fonts->ocraext_message, "Options", white);
    texture[SCORES] = RenderTextBlended(renderer, fonts->ocraext_message, "Scores", white);
    texture[QUIT] = RenderTextBlended(renderer, fonts->ocraext_message, "Quitter", white);

    for(int i = TITLE; i < NUM_TEXT; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;

        if(i != TITLE)
            pos_dst[i].y = 250 + (i - 1) * 80;
        else
            pos_dst[i].y = 110;
    }

    music = Mix_LoadMUS("./data/music/menu.mp3");
    Mix_PlayMusic(music, -1);

    transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 1);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            escape = 1;
        if(KEY_UP_MENU)
        {
            in->key[SDL_SCANCODE_UP] = 0;
            in->controller.hat[0] = SDL_HAT_CENTERED;
            in->controller.axes[1] = 0;

            if(selected > PLAY_GAME)
                selected--;
        }
        if(KEY_DOWN_MENU)
        {
            in->key[SDL_SCANCODE_DOWN] = 0;
            in->controller.hat[0] = SDL_HAT_CENTERED;
            in->controller.axes[1] = 0;

            if(selected < QUIT)
                selected++;
        }
        if(KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller.buttons[0] = 0;

            if(selected == PLAY_GAME)
            {
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 0);
                Mix_HaltMusic();
                Mix_FreeMusic(music);
                playGame(renderer, in, pictures, fonts, sounds, settings);
                music = Mix_LoadMUS("./data/music/menu.mp3");
                Mix_PlayMusic(music, -1);
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, EXITING, 1);
            }

            else if(selected == COMMANDS)
            {
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 0);
                displayKeys(renderer, fonts, pictures, in);
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, EXITING, 1);
            }
            else if(selected == OPTIONS)
            {
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 0);
                displayOptions(renderer, window, pictures, fonts, sounds, settings, in);
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, EXITING, 1);

            }
            else if(selected == SCORES)
            {
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 0);
                unsigned long scores[NUM_SCORES];
                char names[NUM_SCORES][NAME_LEN];
                loadScores(scores, names);
                displayScoreList(renderer, pictures, fonts, in, scores, names);
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, EXITING, 1);
            }
            else if(selected == QUIT)
                escape = 1;
        }
        if(in->key[SDL_SCANCODE_F8])
            editor(renderer, in, pictures, fonts, settings);


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = TITLE; i < NUM_TEXT; i++)
        {
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);
            if(selected == i)
            {
                SDL_Rect pos_arrow;
                SDL_QueryTexture(pictures->HUDlife, NULL, NULL, &pos_arrow.w, &pos_arrow.h);
                pos_arrow.x = pos_dst[i].x - pos_arrow.w - 10;
                pos_arrow.y = pos_dst[i].y + pos_dst[i].h / 2 - pos_arrow.h / 2;
                SDL_RenderCopy(renderer, pictures->HUDlife, NULL, &pos_arrow);
            }
        }

        SDL_RenderPresent(renderer);

        wait(&time1, &time2, DELAY_GAME);
    }

    return EXIT_SUCCESS;
}

