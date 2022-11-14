#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "data.h"
#include "event.h"
#include "key.h"
#include "game.h"
#include "transition.h"


void displayKeys(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, Settings *settings, Sounds *sounds, FPSmanager *fps)
{
    SDL_Texture *texture[NUM_KEYS];
    SDL_Rect pos_dst[NUM_KEYS];
    SDL_Color white = {255, 255, 255, 255};
    int escape = 0, player_selected = 0, player_modified = 0, key_selected = GO_LEFT;
    unsigned long frame_num = 0;
    char str[200] = "";
    SDL_Rect pos_fs;
    pos_fs.x = 0;
    pos_fs.y = 0;
    pos_fs.w = WINDOW_W;
    pos_fs.h = WINDOW_H;

    texture[HEADER] = RenderTextBlended(renderer, fonts->ocraext_message, "Joueur 1                       Joueur 2", white);

    sprintf(str, "Aller à gauche : %s", SDL_GetScancodeName(settings->controls[player_selected].left));
    texture[GO_LEFT] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

    sprintf(str, "Aller à droite : %s", SDL_GetScancodeName(settings->controls[player_selected].right));
    texture[GO_RIGHT] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

    sprintf(str, "Sauter : %s", SDL_GetScancodeName(settings->controls[player_selected].jump));
    texture[JUMP] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

    sprintf(str, "Power-up : %s", SDL_GetScancodeName(settings->controls[player_selected].power_up));
    texture[POWER_UP] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

    texture[RESET_KEYS] = RenderTextBlended(renderer, fonts->ocraext_message, "Réinitialiser les commandes par défaut", white);

    for(int i = HEADER; i < NUM_KEYS; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        if(i == HEADER)
            pos_dst[i].y = 120;
        else if(i < RESET_KEYS)
            pos_dst[i].y = 200 + i * 60;
        else
            pos_dst[i].y = 260 + i * 60;
    }

    transition(renderer, pictures->title, NUM_KEYS, texture, pos_dst, ENTERING, 1, fps);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
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

            if(key_selected != RESET_KEYS)
            {
                SDL_Texture *temp = getScreenTexture(renderer);
                changeKey(renderer, temp, settings, fonts, player_selected, key_selected);
                SDL_DestroyTexture(temp);
            }
            else
            {
                getDefaultControls(settings->controls);
                saveControls(settings->controls);
                Mix_PlayChannel(-1, sounds->enter, 0);
            }

            player_modified = 1;
        }
        if(KEY_LEFT_MENU)
        {
            in->key[SDL_SCANCODE_LEFT] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[0] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[0] = 0;

            if(player_selected > 0)
            {
                 player_selected--;
                 player_modified = 1;
                 Mix_PlayChannel(-1, sounds->select, 0);
            }
        }
        if(KEY_RIGHT_MENU)
        {
            in->key[SDL_SCANCODE_RIGHT] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[0] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[0] = 0;

            if(player_selected < 1)
            {
                player_selected++;
                player_modified = 1;
                Mix_PlayChannel(-1, sounds->select, 0);
            }
        }
        if(KEY_UP_MENU)
        {
            in->key[SDL_SCANCODE_UP] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(key_selected > GO_LEFT)
            {
                key_selected--;
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

            if(key_selected < RESET_KEYS)
            {
                key_selected++;
                Mix_PlayChannel(-1, sounds->select, 0);
            }

        }

        if(player_modified)
        {
            for(int i = GO_LEFT; i <= POWER_UP; i++)
                SDL_DestroyTexture(texture[i]);

            sprintf(str, "Aller à gauche : %s", SDL_GetScancodeName(settings->controls[player_selected].left));
            texture[GO_LEFT] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

            sprintf(str, "Aller à droite : %s", SDL_GetScancodeName(settings->controls[player_selected].right));
            texture[GO_RIGHT] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

            sprintf(str, "Sauter : %s", SDL_GetScancodeName(settings->controls[player_selected].jump));
            texture[JUMP] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

            sprintf(str, "Power-up : %s", SDL_GetScancodeName(settings->controls[player_selected].power_up));
            texture[POWER_UP] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

            for(int i = GO_LEFT; i <= POWER_UP; i++)
            {
                SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
                pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
            }

            player_modified = 0;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, &pos_fs);


        SDL_Rect pos_arrow;
        SDL_QueryTexture(pictures->HUDlife, NULL, NULL, &pos_arrow.w, &pos_arrow.h);
        pos_arrow.x = pos_dst[HEADER].x - pos_arrow.w - 40 + player_selected * 465 + (frame_num % 60 < 30 ? frame_num % 30 : 30 - frame_num % 30);
        pos_arrow.y = pos_dst[HEADER].y + pos_dst[HEADER].h / 2 - pos_arrow.h / 2;
        SDL_RenderCopy(renderer, pictures->HUDlife, NULL, &pos_arrow);

        pos_arrow.x = pos_dst[key_selected].x - pos_arrow.w - 40 + (frame_num % 60 < 30 ? frame_num % 30 : 30 - frame_num % 30);
        pos_arrow.y = pos_dst[key_selected].y + pos_dst[key_selected].h / 2 - pos_arrow.h / 2;
        SDL_RenderCopy(renderer, pictures->HUDlife, NULL, &pos_arrow);

        for(int i = HEADER; i < NUM_KEYS; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        frame_num++;
    }

    transition(renderer, pictures->title, NUM_KEYS, texture, pos_dst, EXITING, 0, fps);

    for(int i = HEADER; i < NUM_KEYS; i++)
        SDL_DestroyTexture(texture[i]);
}


void changeKey(SDL_Renderer *renderer, SDL_Texture *texture, Settings *settings, Fonts *fonts, int player_num, int key)
{
    SDL_Color white = {255, 255, 255, 255};
    SDL_Rect pos_fs;
    pos_fs.x = 0;
    pos_fs.y = 0;
    pos_fs.w = WINDOW_W;
    pos_fs.h = WINDOW_H;

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &pos_fs);
    roundedBoxRGBA(renderer, (int) WINDOW_W / 4, (int) WINDOW_H / 4, ((int) WINDOW_W / 4) * 3, ((int) WINDOW_H / 4) * 3, 10, 64, 64, 64, 245);
    roundedRectangleRGBA(renderer, (int) WINDOW_W / 4, (int) WINDOW_H / 4, ((int) WINDOW_W / 4) * 3, ((int) WINDOW_H / 4) * 3, 10, 255, 255, 255, 245);

    SDL_Texture *text[2];
    text[0] = RenderTextBlended(renderer, fonts->preview_intro, "Appuyez sur une touche", white);
    text[1] = RenderTextBlended(renderer, fonts->ocraext_message, "Retour : Echap", white);

    SDL_Rect pos_dst[2];
    for(int i = 0; i < 2; i++)
    {
        SDL_QueryTexture(text[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        pos_dst[i].y = WINDOW_H / 2 - pos_dst[i].h / 2 - 50 + i * 100;
        SDL_RenderCopy(renderer, text[i], NULL, &pos_dst[i]);
    }

    SDL_RenderPresent(renderer);

    int scancode = getKey();

    if(scancode != SDL_SCANCODE_ESCAPE)
    {
        if(key == GO_LEFT)
            settings->controls[player_num].left = scancode;
        else if(key == GO_RIGHT)
            settings->controls[player_num].right = scancode;
        else if(key == JUMP)
            settings->controls[player_num].jump = scancode;
        else if(key == POWER_UP)
            settings->controls[player_num].power_up = scancode;

        saveControls(settings->controls);
    }

    for(int i = 0; i < 2; i++)
        SDL_DestroyTexture(text[i]);
}

