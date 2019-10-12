#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "file.h"
#include "data.h"
#include "transition.h"


void selectMode(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, const int num_player, Net *net, FPSmanager *fps)
{
    SDL_Color white = {255, 255, 255, 255};
    int selected = 0;
    SDL_Texture *texture[MODES_NUM];
    SDL_Rect pos_dst[MODES_NUM];
    int escape = 0;
    unsigned long frame_num = 0;
    int num_textures;

    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, "Tous les niveaux d'affilée", white);
    SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
    pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;
    pos_dst[0].y = WINDOW_H / 2 - pos_dst[0].h / 2 - 80;

    texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, "Un niveau précis", white);
    SDL_QueryTexture(texture[1], NULL, NULL, &pos_dst[1].w, &pos_dst[1].h);
    pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
    pos_dst[1].y = WINDOW_H / 2 - pos_dst[1].h / 2;

    if(num_player < 2)
    {
        texture[2] = RenderTextBlended(renderer, fonts->ocraext_message, "Scores", white);
        SDL_QueryTexture(texture[2], NULL, NULL, &pos_dst[2].w, &pos_dst[2].h);
        pos_dst[2].x = WINDOW_W / 2 - pos_dst[2].w / 2;
        pos_dst[2].y = WINDOW_H / 2 - pos_dst[2].h / 2 + 80;
        num_textures = 3;
    }
    else
    {
        texture[2] = NULL;
        pos_dst[0].y += 40;
        pos_dst[1].y += 40;
        num_textures = 2;
    }


    transition(renderer, pictures->title, num_textures, texture, pos_dst, ENTERING, 1, fps);

    Packet packet;

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
        {
            if(net != NULL)
            {
                packet.quit = 1;
                packet.choosing = 1;
                strcpy(packet.nickname, settings->nickname);
                SDLNet_TCP_Send(net->client, &packet, sizeof(Packet));
            }

            exit(EXIT_SUCCESS);
        }
        if(KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[1].buttons[6] = 0;

            escape = 1;
        }
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

            if(selected < num_textures - 1)
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

            Mix_PlayChannel(-1, sounds->enter, 0);

            transition(renderer, pictures->title, num_textures, texture, pos_dst, ENTERING, 0, fps);

            if(selected == SCORES)
            {
                unsigned long scores[NUM_SCORES];
                char names[NUM_SCORES][NAME_LEN];
                loadScores(scores, names);
                displayScoreList(renderer, pictures, fonts, in, scores, names, fps);
            }
            else if(selected == ONE_LEVEL)
                map(renderer, in, pictures, fonts, sounds, music, settings, num_player, net, fps);
            else
            {
                transition(renderer, pictures->title, num_textures, texture, pos_dst, ENTERING, 0, fps);

                if(net != NULL)
                {
                    Packet packet;
                    packet.accept = -1;
                    packet.lvl_num = 0;
                    packet.choosing = 1;
                    packet.quit = 0;
                    packet.frame = 0;
                    packet.point.x = SPAWN_X;
                    packet.point.y = SPAWN_Y;
                    packet.state = IDLE_RIGHT;
                    strcpy(packet.nickname, settings->nickname);
                    SDLNet_TCP_Send(net->client, &packet, sizeof(Packet));
                }


                Mix_HaltMusic();
                Mix_FreeMusic(*music);

                playGame(renderer, in, pictures, fonts, sounds, settings, 1, ALL_LEVELS, num_player, net, fps);

                *music = Mix_LoadMUS("data/musics/menu.mp3");
                Mix_PlayMusic(*music, -1);
            }


            transition(renderer, pictures->title, num_textures, texture, pos_dst, EXITING, 1, fps);
        }

        if(net != NULL)
        {
            packet.accept = -1;
            packet.choosing = escape;
            packet.quit = escape;
            strcpy(packet.nickname, settings->nickname);
            SDLNet_TCP_Send(net->client, &packet, sizeof(Packet));
        }


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = 0; i < num_textures; i++)
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

    transition(renderer, pictures->title, num_textures, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < num_textures; i++)
        SDL_DestroyTexture(texture[i]);
}

