#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "data.h"
#include "game.h"
#include "transition.h"
#include "option.h"
#include "key.h"

void displayOptions(SDL_Renderer *renderer, SDL_Window *window, Pictures *pictures, Fonts *fonts, Sounds *sounds, Settings *settings, Input *in, FPSmanager *fps)
{
    SDL_Texture *texture[NUM_OPTIONS];
    SDL_Rect pos_dst[NUM_OPTIONS];
    int escape = 0, selected = 0;
    unsigned long frame_num = 0;


    loadOptionsTexts(renderer, fonts, settings, texture, pos_dst);
    transition(renderer, pictures->title, NUM_OPTIONS, texture, pos_dst, ENTERING, 1, fps);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_UP_MENU)
        {
            in->key[SDL_SCANCODE_UP] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(selected > 0)
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

            if(selected < NUM_OPTIONS - 1)
            {
                selected++;
                Mix_PlayChannel(-1, sounds->select, 0);
            }
        }
        if(KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[1].buttons[6] = 0;
            escape = 1;
        }
        if(KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;

            if(selected == COMMANDS)
            {
                Mix_PlayChannel(-1, sounds->enter, 0);
                transition(renderer, pictures->title, NUM_OPTIONS, texture, pos_dst, ENTERING, 0, fps);
                displayKeys(renderer, fonts, pictures, in, settings, sounds, fps);
                transition(renderer, pictures->title, NUM_OPTIONS, texture, pos_dst, EXITING, 1, fps);
            }
            else if(selected == MODE)
            {
                if(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)
                {
                    settings->fullscreen = 0;
                    SDL_SetWindowFullscreen(window, 0);
                }
                else
                {
                    settings->fullscreen = 1;
                    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                }
            }
            else if(selected == HAPTIC)
            {
                settings->haptic = !settings->haptic;

                if(settings->haptic)
                    for(int i = 0; i < in->num_controller; i++)
                        SDL_HapticRumblePlay(in->controller[i].haptic, 0.5, 500);
            }

            saveSettings(settings);
            destroyOptionsTexts(texture);
            loadOptionsTexts(renderer, fonts, settings, texture, pos_dst);
        }
        if(KEY_LEFT_MENU)
        {
            in->key[SDL_SCANCODE_LEFT] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[0] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[0] = 0;


            if(selected == MUSIC_VOLUME && settings->music_volume > 0)
            {
                settings->music_volume -= 16;
                if(settings->music_volume < 0)
                    settings->music_volume = 0;
                Mix_VolumeMusic(settings->music_volume);
            }
            else if(selected == SFX_VOLUME && settings->sfx_volume > 0)
            {
                settings->sfx_volume -= 16;
                if(settings->sfx_volume < 0)
                    settings->sfx_volume = 0;
                setSfxVolume(sounds, settings->sfx_volume);
                Mix_PlayChannel(-1, sounds->coin, 0);
            }

            saveSettings(settings);
            destroyOptionsTexts(texture);
            loadOptionsTexts(renderer, fonts, settings, texture, pos_dst);
        }
        if(KEY_RIGHT_MENU)
        {
            in->key[SDL_SCANCODE_RIGHT] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[0] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[0] = 0;

            if(selected == MUSIC_VOLUME && settings->music_volume < MIX_MAX_VOLUME)
            {
                settings->music_volume += 16;
                if(settings->music_volume > 128)
                    settings->music_volume = 128;
                Mix_VolumeMusic(settings->music_volume);
            }
            else if(selected == SFX_VOLUME && settings->sfx_volume < MIX_MAX_VOLUME)
            {
                settings->sfx_volume += 16;
                if(settings->sfx_volume > 128)
                    settings->sfx_volume = 128;
                setSfxVolume(sounds, settings->sfx_volume);
                Mix_PlayChannel(-1, sounds->coin, 0);
            }

            saveSettings(settings);
            destroyOptionsTexts(texture);
            loadOptionsTexts(renderer, fonts, settings, texture, pos_dst);
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = 0; i < NUM_OPTIONS; i++)
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

    transition(renderer, pictures->title, NUM_OPTIONS, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < NUM_OPTIONS; i++)
        SDL_DestroyTexture(texture[i]);
}



void destroyOptionsTexts(SDL_Texture *texture[NUM_OPTIONS])
{
    for(int i = 0; i < NUM_OPTIONS; i++)
        SDL_DestroyTexture(texture[i]);
}


void loadOptionsTexts(SDL_Renderer *renderer, Fonts *fonts, Settings *settings, SDL_Texture *texture[NUM_OPTIONS], SDL_Rect pos_dst[NUM_OPTIONS])
{
    char str[100] = "";
    SDL_Color white = {255, 255, 255};

    texture[MODE] = RenderTextBlended(renderer, fonts->ocraext_message, (settings->fullscreen) ? "Plein écran : Oui" : "Plein écran : Non", white);

    texture[COMMANDS] = RenderTextBlended(renderer, fonts->ocraext_message, "Commandes...", white);

    sprintf(str, "Volume de la musique :  %s  %.1f %%  %s", (settings->music_volume > 0) ? "<" : " ", (settings->music_volume * 100.0) / 128.0, (settings->music_volume < 128) ? ">" : "");
    texture[MUSIC_VOLUME] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

    sprintf(str, "Volume des sons :  %s  %.1f %%  %s", (settings->sfx_volume > 0) ? "<" : " ", (settings->sfx_volume * 100.0) / 128.0, (settings->sfx_volume < 128) ? ">" : "");
    texture[SFX_VOLUME] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

    texture[HAPTIC] = RenderTextBlended(renderer, fonts->ocraext_message, (settings->haptic) ? "Vibrations manette : Oui" : "Vibrations manette : Non", white);

    for(int i = 0; i < NUM_OPTIONS; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        pos_dst[i].y = 170 + (i * 80);
    }
}


void setSfxVolume(Sounds *sounds, int volume)
{
    Mix_VolumeChunk(sounds->bumper, volume);
    Mix_VolumeChunk(sounds->checkpoint, volume);
    Mix_VolumeChunk(sounds->coin, volume);
    Mix_VolumeChunk(sounds->death, volume);
    Mix_VolumeChunk(sounds->explosion, volume);
    Mix_VolumeChunk(sounds->invicible, volume);
    Mix_VolumeChunk(sounds->jump, volume);
    Mix_VolumeChunk(sounds->life, volume);
    Mix_VolumeChunk(sounds->complete, volume);
    Mix_VolumeChunk(sounds->select, volume);
    Mix_VolumeChunk(sounds->enter, volume);
    Mix_VolumeChunk(sounds->gun, volume);
}


