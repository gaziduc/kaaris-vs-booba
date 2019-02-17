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





void levelFinished(SDL_Renderer *renderer, Sounds *sounds, Fonts *fonts, SDL_Texture *texture)
{
    unsigned long time1 = 0, time2 = 0;
    int y = WINDOW_H;
    float speed = 82;
    SDL_Color blue = {0, 0, 255};
    SDL_Texture *text = RenderTextBlended(renderer, fonts->ocraext_title, "Niveau terminé !", blue);
    SDL_Rect pos_dst;
    SDL_QueryTexture(text, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.x = WINDOW_W / 2 - pos_dst.w / 2;
    pos_dst.y = y + WINDOW_H / 4 - pos_dst.h / 2;

    Mix_PlayChannel(-1, sounds->complete, 0);

    while(y > WINDOW_H / 4)
    {
        speed /= 1.15;
        if(speed < 1)
            speed *= 2;
        y -= (int) (speed + 0.5);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        roundedBoxRGBA(renderer, WINDOW_W / 6, y, (WINDOW_W / 6) * 5, y + WINDOW_H / 2, 50, 128, 255, 128, 255);
        roundedRectangleRGBA(renderer, WINDOW_W / 6, y, (WINDOW_W / 6) * 5, y + WINDOW_H / 2, 50, 0, 0, 255, 255);

        pos_dst.y = y + WINDOW_H / 4 - pos_dst.h / 2;
        SDL_RenderCopy(renderer, text, NULL, &pos_dst);
        SDL_RenderPresent(renderer);

        wait(&time1, &time2, DELAY_GAME);
    }

    SDL_Delay(2000);
    SDL_DestroyTexture(text);
}



