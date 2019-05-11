#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "transition.h"
#include "game.h"

void transition(SDL_Renderer *renderer, SDL_Texture *bg, int num_textures, SDL_Texture *texture[], SDL_Rect pos_dst[], const int type, const int number)
{
    float speed = 0;
    int finished = 0;
    unsigned long time1 = 0, time2 = 0;

    if(type == ENTERING)
    {
        if(number == 0)
            speed = -1;
        else
        {
            speed = -MAX_SPEED;
            for(int i = 0; i < num_textures; i++)
                pos_dst[i].x = WINDOW_W - pos_dst[i].w / 2;
        }
    }
    else
    {
        if(number == 0)
            speed = 1;
        else
        {
            speed = MAX_SPEED;
            for(int i = 0; i < num_textures; i++)
                pos_dst[i].x = -pos_dst[i].w / 2;
        }
    }


    while(!finished)
    {
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, bg, NULL, NULL);

        if(number == 0)
            speed *= 1.15;
        else
            speed /= 1.15;

        if(speed > MAX_SPEED)
            speed = MAX_SPEED;
        else if(speed > -1 && speed < 1)
            speed *= 2;

        finished = 1;

        for(int i = 0; i < num_textures; i++)
        {
            pos_dst[i].x += (int) (speed + 0.5);

            if(type == ENTERING && number == 0)
            {
                if(pos_dst[i].x + pos_dst[i].w / 2 > 0)
                    finished = 0;
            }
            else if(type == EXITING && number == 0)
            {
                if(pos_dst[i].x + pos_dst[i].w / 2 < WINDOW_W)
                    finished = 0;
            }
            else if(pos_dst[i].x + pos_dst[i].w / 2 < WINDOW_W / 2 - 2 || pos_dst[i].x + pos_dst[i].w / 2 > WINDOW_W / 2 + 2)
                finished = 0;

            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);
        }

        SDL_RenderPresent(renderer);

        waitGame(&time1, &time2, DELAY_GAME);
    }

}



void levelFinished(SDL_Renderer *renderer, Fonts *fonts, Player *player, int level_num, int num_player, int player_num, int lvl_finished[])
{
    SDL_Color white = {255, 255, 255};
    SDL_Texture *texture = NULL;
    SDL_Rect pos_dst;
    char str[100] = "";

    pos_dst.x = WINDOW_W / 6;
    pos_dst.y = (num_player < 2) ? WINDOW_H / 4 + 50 : player_num * (WINDOW_H / 2) + 50;
    pos_dst.w = WINDOW_W / 1.5;
    pos_dst.h = WINDOW_H / 2 - 100;

    SDL_SetRenderDrawColor(renderer, 100, 90, 120, 192);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(renderer, &pos_dst);
    SDL_SetRenderDrawColor(renderer, 45, 200, 100, 255);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderDrawRect(renderer, &pos_dst);

    sprintf(str, "Niveau %d terminé !", level_num);
    texture = RenderTextBlended(renderer, fonts->ocraext_score, str, white);
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.x = WINDOW_W / 2 - pos_dst.w / 2;
    pos_dst.y += 50;
    SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
    SDL_DestroyTexture(texture);

    sprintf(str, "Temps : %lu.%lu%lu%lu secondes", player->timer / 1000, (player->timer % 1000 < 100) ? 0 : (player->timer % 1000) / 100, (player->timer % 100 < 10) ? 0 : (player->timer % 100) / 10, (player->timer % 10 == 0) ? 0 : player->timer % 10);
    texture = RenderTextBlended(renderer, fonts->ocraext_commands, str, white);
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.x = WINDOW_W / 2 - pos_dst.w / 2;
    pos_dst.y += 80;
    SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
    SDL_DestroyTexture(texture);

    int finished = 1;

    for(int i = 0; i < num_player; i++)
        if(!lvl_finished[i])
            finished = 0;

    texture = RenderTextBlended(renderer, fonts->ocraext_commands, (finished) ? "ENTREE pour continuer" : "En attente de l'autre joueur", white);
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.x = WINDOW_W / 2 - pos_dst.w / 2;
    pos_dst.y += 65;
    SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
    SDL_DestroyTexture(texture);
}


int pauseGame(SDL_Renderer *renderer, SDL_Texture *texture, Pictures *pictures, Fonts *fonts, Input *in)
{
    SDL_Color black = {0, 0, 0};
    int selected = RESUME;
    int escape = 0;
    unsigned long time1 = 0, time2 = 0;

    SDL_Texture *text[3];
    text[0] = RenderTextBlended(renderer, fonts->ocraext_title, "Pause", black);
    text[1] = RenderTextBlended(renderer, fonts->ocraext_message, "Reprendre", black);
    text[2] = RenderTextBlended(renderer, fonts->ocraext_message, "Quitter", black);

    SDL_Rect pos_dst[3];

    for(int i = 0; i < 3; i++)
    {
        SDL_QueryTexture(text[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;

        if(i != 0)
            pos_dst[i].y = 370 + (i - 1) * 80;
        else
            pos_dst[i].y = 240;
    }


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

            if(selected > RESUME)
                selected--;
        }
        if(KEY_DOWN_MENU)
        {
            in->key[SDL_SCANCODE_DOWN] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(selected < QUIT_PAUSE)
                selected++;
        }
        if(KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;

            if(selected == RESUME)
                escape = 1;
            else if(selected == QUIT_PAUSE)
                escape = 2;
        }
        if(KEY_ESCAPE || KEY_PAUSE)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->key[SDL_SCANCODE_P] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[0].buttons[7] = 0;
            in->controller[1].buttons[6] = 0;
            in->controller[1].buttons[7] = 0;

            escape = 1;
        }


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        roundedBoxRGBA(renderer, WINDOW_W / 6, WINDOW_H / 4, (WINDOW_W / 6) * 5, (WINDOW_H / 4) * 3, 25, 128, 128, 128, 192);
        roundedRectangleRGBA(renderer, WINDOW_W / 6, WINDOW_H / 4, (WINDOW_W / 6) * 5, (WINDOW_H / 4) * 3, 25, 240, 240, 240, 255);

        for(int i = 0; i < NUM_TEXT_PAUSE; i++)
        {
            SDL_RenderCopy(renderer, text[i], NULL, &pos_dst[i]);
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

    for(int i = 0; i < 3; i++)
        SDL_DestroyTexture(text[i]);

    return escape;
}



