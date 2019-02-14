#include <SDL2/SDL.h>
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
                if(pos_dst[i].x > 0)
                    finished = 0;
            }
            else if(type == EXITING && number == 0)
            {
                if(pos_dst[i].x < WINDOW_W)
                    finished = 0;
            }
            else if(pos_dst[i].x + pos_dst[i].w / 2 < WINDOW_W / 2 - 2 || pos_dst[i].x + pos_dst[i].w / 2 > WINDOW_W / 2 + 2)
                finished = 0;

            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);
        }

        SDL_RenderPresent(renderer);

        wait(&time1, &time2, DELAY_GAME);
    }

}

