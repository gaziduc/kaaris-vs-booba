#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "data.h"
#include "event.h"
#include "key.h"
#include "game.h"
#include "transition.h"


void displayKeys(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, Settings *settings)
{
    SDL_Texture *texture[NUM_KEYS];
    SDL_Rect pos_dst[NUM_KEYS];
    SDL_Color white = {255, 255, 255};
    int escape = 0, player_selected = 0, player_modified = 0, key_selected = GO_LEFT;
    unsigned long time1 = 0, time2 = 0;
    char str[200] = "";

    texture[HEADER] = RenderTextBlended(renderer, fonts->ocraext_score, "Joueur 1                 Joueur 2", white);

    char *scancodeToString = SDL_GetScancodeName(settings->controls[player_selected].left);
    sprintf(str, "Aller à gauche : %s", scancodeToString);
    texture[GO_LEFT] = RenderTextBlended(renderer, fonts->ocraext_score, str, white);

    scancodeToString = SDL_GetScancodeName(settings->controls[player_selected].right);
    sprintf(str, "Aller à droite : %s", scancodeToString);
    texture[GO_RIGHT] = RenderTextBlended(renderer, fonts->ocraext_score, str, white);

    scancodeToString = SDL_GetScancodeName(settings->controls[player_selected].jump);
    sprintf(str, "Sauter : %s", scancodeToString);
    texture[JUMP] = RenderTextBlended(renderer, fonts->ocraext_score, str, white);

    for(int i = HEADER; i < NUM_KEYS; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        if(i == HEADER)
            pos_dst[i].y = 120;
        else
            pos_dst[i].y = 200 + (i * 70);
    }

    transition(renderer, pictures->title, NUM_KEYS, texture, pos_dst, ENTERING, 1);

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

            SDL_Texture *temp = getScreenTexture(renderer);
            changeKey(renderer, temp, settings, fonts, player_selected, key_selected);
            SDL_DestroyTexture(temp);

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
                key_selected--;
        }
        if(KEY_DOWN_MENU)
        {
            in->key[SDL_SCANCODE_DOWN] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(key_selected < JUMP)
                key_selected++;
        }

        if(player_modified)
        {
            for(int i = GO_LEFT; i <= JUMP; i++)
                SDL_DestroyTexture(texture[i]);

            char *scancodeToString = SDL_GetScancodeName(settings->controls[player_selected].left);
            sprintf(str, "Aller à gauche : %s", scancodeToString);
            texture[GO_LEFT] = RenderTextBlended(renderer, fonts->ocraext_score, str, white);

            scancodeToString = SDL_GetScancodeName(settings->controls[player_selected].right);
            sprintf(str, "Aller à droite : %s", scancodeToString);
            texture[GO_RIGHT] = RenderTextBlended(renderer, fonts->ocraext_score, str, white);

            scancodeToString = SDL_GetScancodeName(settings->controls[player_selected].jump);
            sprintf(str, "Sauter : %s", scancodeToString);
            texture[JUMP] = RenderTextBlended(renderer, fonts->ocraext_score, str, white);

            for(int i = GO_LEFT; i <= JUMP; i++)
            {
                SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
                pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
            }

            player_modified = 0;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        roundedBoxRGBA(renderer, pos_dst[HEADER].x + player_selected * 520 - 55, pos_dst[HEADER].y - 20, pos_dst[HEADER].x + player_selected * 520 + 240 - 15, pos_dst[HEADER].y + 60, 25, 82, 171, 78, 255);
        roundedBoxRGBA(renderer, (int) WINDOW_W / 2 - 400, pos_dst[key_selected].y - 20, (int) WINDOW_W / 2 + 400, pos_dst[key_selected].y + 55, 25, 0, 160, 160, 255);

        for(int i = HEADER; i < NUM_KEYS; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);

        waitGame(&time1, &time2, DELAY_GAME);
    }

    transition(renderer, pictures->title, NUM_KEYS, texture, pos_dst, EXITING, 0);

    for(int i = HEADER; i < NUM_KEYS; i++)
        SDL_DestroyTexture(texture[i]);
}


void changeKey(SDL_Renderer *renderer, SDL_Texture *texture, Settings *settings, Fonts *fonts, int player_num, int key)
{
    SDL_Color white = {255, 255, 255};
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    roundedBoxRGBA(renderer, (int) WINDOW_W / 4, (int) WINDOW_H / 4, ((int) WINDOW_W / 4) * 3, ((int) WINDOW_H / 4) * 3, 50, 64, 64, 64, 250);
    SDL_Texture *text = RenderTextBlended(renderer, fonts->ocraext_message, "Appuyez sur une touche...", white);
    SDL_Rect pos_dst;
    SDL_QueryTexture(text, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.x = WINDOW_W / 2 - pos_dst.w / 2;
    pos_dst.y = WINDOW_H / 2 - pos_dst.h / 2;
    SDL_RenderCopy(renderer, text, NULL, &pos_dst);
    SDL_RenderPresent(renderer);

    int scancode = getKey();

    if(key == GO_LEFT)
        settings->controls[player_num].left = scancode;
    else if(key == GO_RIGHT)
        settings->controls[player_num].right = scancode;
    else if(key == JUMP)
        settings->controls[player_num].jump = scancode;

    saveSettings(settings);

    SDL_DestroyTexture(text);
}

