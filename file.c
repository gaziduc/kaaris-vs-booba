#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "file.h"
#include "data.h"
#include "transition.h"


void selectMode(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, const int num_player, Net *net)
{
    SDL_Color white = {255, 255, 255};
    int selected = 0;
    SDL_Texture *texture[MODES_NUM];
    SDL_Rect pos_dst[MODES_NUM];
    int escape = 0;
    unsigned long time1 = 0, time2 = 0, frame_num = 0;
    int goUp = 1;

    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, "Tous les niveaux d'affilés", white);
    SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
    pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;
    pos_dst[0].y = 290;

    texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, "Un niveau précis", white);
    SDL_QueryTexture(texture[1], NULL, NULL, &pos_dst[1].w, &pos_dst[1].h);
    pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
    pos_dst[1].y = 390;

    transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 1);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
        {
            if(net != NULL)
            {
                ChoosePacket packet;
                packet.level_num = -1;
                SDLNet_TCP_Send(net->client, &packet, sizeof(ChoosePacket));
            }

            exit(EXIT_SUCCESS);
        }
        if(KEY_ESCAPE)
        {
            if(net != NULL)
            {
                ChoosePacket packet;
                packet.level_num = -1;
                SDLNet_TCP_Send(net->client, &packet, sizeof(ChoosePacket));
            }

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
                selected--;
        }
        if(KEY_DOWN_MENU)
        {
            in->key[SDL_SCANCODE_DOWN] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(selected < MODES_NUM - 1)
                selected++;
        }
        if(KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;

            transition(renderer, pictures->title, MODES_NUM, texture, pos_dst, ENTERING, 0);

            if(selected == ONE_LEVEL)
                map(renderer, in, pictures, fonts, sounds, music, settings, num_player, net);
            else
            {
                if(net != NULL)
                {
                    ChoosePacket packet;
                    packet.level_num = 0;
                    SDLNet_TCP_Send(net->client, &packet, sizeof(ChoosePacket));
                }


                Mix_HaltMusic();
                Mix_FreeMusic(*music);

                playGame(renderer, in, pictures, fonts, sounds, settings, 1, ALL_LEVELS, num_player, net);

                *music = Mix_LoadMUS("./data/music/menu.mp3");
                Mix_PlayMusic(*music, -1);
            }


            transition(renderer, pictures->title, MODES_NUM, texture, pos_dst, EXITING, 1);
        }


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = 0; i < MODES_NUM; i++)
        {
            roundedBoxRGBA(renderer, WINDOW_W / 4 - 60, pos_dst[i].y - 20, (WINDOW_W / 4) * 3 + 60, pos_dst[i].y + pos_dst[i].h + 20, 25, 0, (selected == i) ? 140 - frame_num : 162, (i == 1) ? 0 : 232, 255);
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);
            if(selected == i)
            {
                SDL_Rect pos_arrow;
                SDL_QueryTexture(pictures->HUDlife, NULL, NULL, &pos_arrow.w, &pos_arrow.h);
                pos_arrow.x = pos_dst[i].x - pos_arrow.w - 20;
                pos_arrow.y = pos_dst[i].y + pos_dst[i].h / 2 - pos_arrow.h / 2;
                SDL_RenderCopy(renderer, pictures->HUDlife, NULL, &pos_arrow);
            }
        }

        SDL_RenderPresent(renderer);

        waitGame(&time1, &time2, DELAY_GAME);
        if(goUp)
        {
            frame_num += 2;
            if(frame_num >= 80)
                goUp = 0;
        }
        else
        {
            frame_num -= 2;
            if(frame_num <= 0)
                goUp = 1;
        }
    }

    transition(renderer, pictures->title, MODES_NUM, texture, pos_dst, EXITING, 0);

    for(int i = 0; i < MODES_NUM; i++)
        SDL_DestroyTexture(texture[i]);
}

