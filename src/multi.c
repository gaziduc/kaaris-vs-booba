#include <stdio.h>
#include <SDL2/SDL.h>
#include "event.h"
#include "data.h"
#include "multi.h"
#include "game.h"
#include "transition.h"
#include "file.h"

void selectMultiCommandType(SDL_Renderer *renderer, Input *in, Fonts *fonts, Pictures *pictures, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps)
{
    SDL_Color white = {255, 255, 255, 255};
    SDL_Texture *texture[NUM_TEXT_MULTI];
    SDL_Rect pos_dst[NUM_TEXT_MULTI];
    int escape = 0;
    char str[200] = "";
    SDL_Rect pos_fs;
    pos_fs.x = 0;
    pos_fs.y = 0;
    pos_fs.w = WINDOW_W;
    pos_fs.h = WINDOW_H;

    texture[START_GAME] = RenderTextBlended(renderer, fonts->ocraext_score, "APPUYEZ SUR ENTREE POUR COMMENCER !", white);

    sprintf(str, "Joueur 1 : Clavier / %s", (in->controller[0].joystick == NULL) ? "Aucune manette détectée" : SDL_JoystickName(in->controller[0].joystick));
    texture[P1_TYPE] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

    sprintf(str, "Joueur 2 : Clavier / %s", (in->controller[1].joystick == NULL) ? "Aucune manette détectée" : SDL_JoystickName(in->controller[1].joystick));
    texture[P2_TYPE] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);


    for(int i = 0; i < NUM_TEXT_MULTI; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;

        if(i == START_GAME)
            pos_dst[i].y = 200;
        else
            pos_dst[i].y = 320 + 70 * i;
    }

    transition(renderer, pictures->title, NUM_TEXT_MULTI, texture, pos_dst, ENTERING, 1, fps);

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
            in->controller[0].buttons[1] = 0;
            in->controller[1].buttons[0] = 0;
            in->controller[1].buttons[1] = 0;

            Mix_PlayChannel(-1, sounds->enter, 0);

            transition(renderer, pictures->title, NUM_TEXT_MULTI, texture, pos_dst, ENTERING, 0, fps);
            selectMode(renderer, pictures, fonts, in, sounds, music, settings, 2, NULL, fps);
            transition(renderer, pictures->title, NUM_TEXT_MULTI, texture, pos_dst, EXITING, 1, fps);
        }

        SDL_DestroyTexture(texture[P1_TYPE]);
        SDL_DestroyTexture(texture[P2_TYPE]);

        sprintf(str, "Joueur 1 : Clavier / %s", (in->controller[0].joystick == NULL) ? "Aucune manette détectée" : SDL_JoystickName(in->controller[0].joystick));
        texture[P1_TYPE] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

        sprintf(str, "Joueur 2 : Clavier / %s", (in->controller[1].joystick == NULL) ? "Aucune manette détectée" : SDL_JoystickName(in->controller[1].joystick));
        texture[P2_TYPE] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

        for(int i = 0; i < NUM_TEXT_MULTI; i++)
        {
            SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
            pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        }


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, &pos_fs);
        for(int i = 0; i < NUM_TEXT_MULTI; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);
    }

    transition(renderer, pictures->title, NUM_TEXT_MULTI, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < NUM_TEXT_MULTI; i++)
        SDL_DestroyTexture(texture[i]);
}

