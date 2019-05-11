#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_net.h>
#include "event.h"
#include "game.h"
#include "key.h"
#include "transition.h"
#include "editor.h"
#include "option.h"
#include "file.h"
#include "multi.h"

enum {TITLE, SOLO, MULTIPLAYER, ONLINE, COMMANDS, OPTIONS, SCORES, QUIT, NUM_TEXT};

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
    int selected = SOLO;


    srand(time(NULL));

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC) < 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2", SDL_GetError(), NULL);
        exit(EXIT_FAILURE);
    }

    window = SDL_CreateWindow("Kaaris vs Booba", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    if(window == NULL)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not create window", SDL_GetError(), NULL);
        exit(EXIT_FAILURE);
    }

    SDL_ShowCursor(SDL_DISABLE);

    renderer = SDL_CreateRenderer(window, -1, 0);
    if(renderer == NULL)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not create renderer", SDL_GetError(), window);
        exit(EXIT_FAILURE);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

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
    #ifdef WIN64
        if((initted & mix_flags) != mix_flags)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2_mixer", Mix_GetError(), window);
            exit(EXIT_FAILURE);
        }
    #endif // WIN64

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    Mix_AllocateChannels(16); // 16 to be sure that all sfx will be heard (multiple coins, ...)

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

    icon = IMG_Load("./data/hud/life.bmp");
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);

    if(settings->fullscreen)
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    texture[TITLE] = RenderTextBlended(renderer, fonts->ocraext_title, "Kaaris vs Booba", white);
    texture[SOLO] = RenderTextBlended(renderer, fonts->ocraext_message, "Solo", white);
    texture[MULTIPLAYER] = RenderTextBlended(renderer, fonts->ocraext_message, "Multijoueurs", white);
    texture[ONLINE] = RenderTextBlended(renderer, fonts->ocraext_message, "En ligne", white);
    texture[COMMANDS] = RenderTextBlended(renderer, fonts->ocraext_message, "Commandes", white);
    texture[OPTIONS] = RenderTextBlended(renderer, fonts->ocraext_message, "Options", white);
    texture[SCORES] = RenderTextBlended(renderer, fonts->ocraext_message, "Scores", white);
    texture[QUIT] = RenderTextBlended(renderer, fonts->ocraext_message, "Quitter", white);

    for(int i = TITLE; i < NUM_TEXT; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;

        if(i != TITLE)
            pos_dst[i].y = 240 + (i - 1) * 60;
        else
            pos_dst[i].y = 100;
    }

    music = Mix_LoadMUS("./data/music/menu.mp3");
    Mix_PlayMusic(music, -1);


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
                selected--;
        }
        if(KEY_DOWN_MENU)
        {
            in->key[SDL_SCANCODE_DOWN] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(selected < QUIT)
                selected++;
        }
        if(KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;

            if(selected == SOLO)
            {
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 0);
                selectMode(renderer, pictures, fonts, in, sounds, &music, settings, 1, NULL);
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, EXITING, 1);
            }
            else if(selected == MULTIPLAYER)
            {
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 0);
                selectMultiCommandType(renderer, in, fonts, pictures, sounds, &music, settings);
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, EXITING, 1);
            }
            else if(selected == ONLINE)
            {
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 0);
                hostOrJoin(renderer, pictures, fonts, in, sounds, &music, settings);
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, EXITING, 1);
            }
            else if(selected == COMMANDS)
            {
                transition(renderer, pictures->title, NUM_TEXT, texture, pos_dst, ENTERING, 0);
                displayKeys(renderer, fonts, pictures, in, settings);
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

        waitGame(&time1, &time2, DELAY_GAME);
    }

    return EXIT_SUCCESS;
}

