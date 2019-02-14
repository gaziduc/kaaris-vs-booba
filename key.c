#include <stdio.h>
#include <SDL2/SDL.h>
#include "data.h"
#include "event.h"
#include "key.h"
#include "game.h"
#include "transition.h"


void displayKeys(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in)
{
    SDL_Texture *texture[NUM_KEYS];
    SDL_Texture *no_controller = NULL;
    SDL_Texture *controller = NULL;
    SDL_Rect pos_dst[NUM_KEYS];
    SDL_Rect pos_controller;
    SDL_Color white = {255, 255, 255};
    SDL_Color green = {0, 220, 0};
    int escape = 0;
    unsigned long time1 = 0, time2 = 0;
    char str[200] = "";

    texture[HEADER]     = RenderTextBlended(renderer, fonts->ocraext_commands, "                     |        Clavier         |                 Manette               ", white);
    texture[HLINE]      = RenderTextBlended(renderer, fonts->ocraext_commands, "_____________________|________________________|_______________________________________", white);
    texture[GO_LEFT]    = RenderTextBlended(renderer, fonts->ocraext_commands, "  Aller à gauche     |    Flèche de gauche    |  Jostick gauche axe X vers la gauche  ", white);
    texture[GO_RIGHT]   = RenderTextBlended(renderer, fonts->ocraext_commands, "  Aller à droite     |    Flèche de droite    |  Jostick gauche axe X vers la droite  ", white);
    texture[JUMP]       = RenderTextBlended(renderer, fonts->ocraext_commands, "  Sauter             |    Flèche du haut      |  Button 0   (Xbox 360 : A)            ", white);
    texture[BACK]       = RenderTextBlended(renderer, fonts->ocraext_commands, "  Retour             |    Echap               |  Button 6   (Xbox 360 : Back/Select)  ", white);
    texture[VLINE0]     = RenderTextBlended(renderer, fonts->ocraext_commands, "                     |                        |                                       ", white);
    texture[VLINE1]     = RenderTextBlended(renderer, fonts->ocraext_commands, "                     |                        |                                       ", white);
    texture[VLINE2]     = RenderTextBlended(renderer, fonts->ocraext_commands, "                     |                        |                                       ", white);
    texture[VLINE3]     = RenderTextBlended(renderer, fonts->ocraext_commands, "                     |                        |                                       ", white);
    texture[VLINE4]     = RenderTextBlended(renderer, fonts->ocraext_commands, "                     |                        |                                       ", white);
    texture[VLINE5]     = RenderTextBlended(renderer, fonts->ocraext_commands, "                     |                        |                                       ", white);
    no_controller       = RenderTextBlended(renderer, fonts->ocraext_message, "Aucune manette détectée...", white);

    for(int i = HEADER; i < NUM_KEYS; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        pos_dst[i].y = 160 + (i * 15);
    }

    transition(renderer, pictures->title, NUM_KEYS, texture, pos_dst, ENTERING, 1);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ESCAPE || KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller.buttons[6] = 0;
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller.buttons[0] = 0;
            escape = 1;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = HEADER; i < NUM_KEYS; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        if(in->controller.joystick != NULL)
        {
            sprintf(str, "Manette détectée : %s", SDL_JoystickName(in->controller.joystick));
            controller = RenderTextBlended(renderer, fonts->ocraext_message, str, green);
            SDL_QueryTexture(controller, NULL, NULL, &pos_controller.w, &pos_controller.h);
            pos_controller.x = WINDOW_W / 2 - pos_controller.w / 2;
            pos_controller.y = 500;
            SDL_RenderCopy(renderer, controller, NULL, &pos_controller);
            SDL_DestroyTexture(controller);

            sprintf(str, "Supporte les vibrations : %s", (in->controller.haptic != NULL) ? "Oui" : "Non");
            controller = RenderTextBlended(renderer, fonts->ocraext_message, str, green);
            SDL_QueryTexture(controller, NULL, NULL, &pos_controller.w, &pos_controller.h);
            pos_controller.x = WINDOW_W / 2 - pos_controller.w / 2;
            pos_controller.y = 540;
            SDL_RenderCopy(renderer, controller, NULL, &pos_controller);
            SDL_DestroyTexture(controller);
        }
        else
        {
            SDL_QueryTexture(no_controller, NULL, NULL, &pos_controller.w, &pos_controller.h);
            pos_controller.x = WINDOW_W / 2 - pos_controller.w / 2;
            pos_controller.y = 500;
            SDL_RenderCopy(renderer, no_controller, NULL, &pos_controller);
        }

        SDL_RenderPresent(renderer);

        wait(&time1, &time2, DELAY_GAME);
    }

    transition(renderer, pictures->title, NUM_KEYS, texture, pos_dst, EXITING, 0);

    for(int i = HEADER; i < NUM_KEYS; i++)
        SDL_DestroyTexture(texture[i]);

    SDL_DestroyTexture(no_controller);
}

