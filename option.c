#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "data.h"
#include "game.h"
#include "transition.h"
#include "option.h"

void displayOptions(SDL_Renderer *renderer, SDL_Window *window, Pictures *pictures, Fonts *fonts, Sounds *sounds, Settings *settings, Input *in)
{
    SDL_Texture *texture[NUM_OPTIONS];
    SDL_Rect pos_dst[NUM_OPTIONS];
    int escape = 0, selected = MODE;
    unsigned long time1 = 0, time2 = 0;


    loadOptionsTexts(renderer, fonts, settings, texture, pos_dst);
    transition(renderer, pictures->title, NUM_OPTIONS, texture, pos_dst, ENTERING, 1);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_UP_MENU)
        {
            in->key[SDL_SCANCODE_UP] = 0;
            in->controller.hat[0] = SDL_HAT_CENTERED;
            in->controller.axes[1] = 0;

            if(selected > MODE)
                selected--;
        }
        if(KEY_DOWN_MENU)
        {
            in->key[SDL_SCANCODE_DOWN] = 0;
            in->controller.hat[0] = SDL_HAT_CENTERED;
            in->controller.axes[1] = 0;

            if(selected < SFX_VOLUME)
                selected++;
        }
        if(KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller.buttons[6] = 0;
            escape = 1;
        }
        if(KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller.buttons[0] = 0;

            if(selected == MODE)
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

            saveSettings(settings);
            destroyOptionsTexts(texture);
            loadOptionsTexts(renderer, fonts, settings, texture, pos_dst);
        }
        if(KEY_LEFT_GAME)
        {
            in->key[SDL_SCANCODE_LEFT] = 0;
            in->controller.hat[0] = SDL_HAT_CENTERED;
            in->controller.axes[0] = 0;


            if(selected == MUSIC_VOLUME && settings->music_volume > 0)
            {
                settings->music_volume--;
                Mix_VolumeMusic(settings->music_volume);
            }
            else if(selected == SFX_VOLUME && settings->sfx_volume > 0)
            {
                settings->sfx_volume--;
                setSfxVolume(sounds, settings->sfx_volume);
                Mix_PlayChannel(-1, sounds->coin, 0);
            }

            saveSettings(settings);
            destroyOptionsTexts(texture);
            loadOptionsTexts(renderer, fonts, settings, texture, pos_dst);
        }
        if(KEY_RIGHT_GAME)
        {
            in->key[SDL_SCANCODE_RIGHT] = 0;
            in->controller.hat[0] = SDL_HAT_CENTERED;
            in->controller.axes[0] = 0;

            if(selected == MUSIC_VOLUME && settings->music_volume < MIX_MAX_VOLUME)
            {
                settings->music_volume++;
                Mix_VolumeMusic(settings->music_volume);
            }
            else if(selected == SFX_VOLUME && settings->sfx_volume < MIX_MAX_VOLUME)
            {
                settings->sfx_volume++;
                setSfxVolume(sounds, settings->sfx_volume);
                Mix_PlayChannel(-1, sounds->coin, 0);
            }

            saveSettings(settings);
            destroyOptionsTexts(texture);
            loadOptionsTexts(renderer, fonts, settings, texture, pos_dst);
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = MODE; i < NUM_OPTIONS; i++)
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

    transition(renderer, pictures->title, NUM_OPTIONS, texture, pos_dst, EXITING, 0);

    for(int i = MODE; i < NUM_OPTIONS; i++)
        SDL_DestroyTexture(texture[i]);
}



void destroyOptionsTexts(SDL_Texture *texture[NUM_OPTIONS])
{
    for(int i = MODE; i < NUM_OPTIONS; i++)
        SDL_DestroyTexture(texture[i]);
}


void loadOptionsTexts(SDL_Renderer *renderer, Fonts *fonts, Settings *settings, SDL_Texture *texture[NUM_OPTIONS], SDL_Rect pos_dst[NUM_OPTIONS])
{
    char str[100] = "";
    SDL_Color white = {255, 255, 255};

    texture[MODE] = RenderTextBlended(renderer, fonts->ocraext_message, (settings->fullscreen) ? "Plein écran : Oui" : "Plein écran : Non", white);

    sprintf(str, "Volume de la musique : %.1f %%", (settings->music_volume * 100.0) / 128.0);
    texture[MUSIC_VOLUME] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

    sprintf(str, "Volume des sons : %.1f %%", (settings->sfx_volume * 100.0) / 128.0);
    texture[SFX_VOLUME] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

    for(int i = MODE; i < NUM_OPTIONS; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        pos_dst[i].y = 250 + (i * 80);
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
    Mix_VolumeChunk(sounds->text, volume);
    Mix_VolumeChunk(sounds->complete, volume);
}


