#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "event.h"
#include "data.h"
#include "text.h"
#include "game.h"

void intro(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds)
{
    textIntro(renderer, in, pictures, fonts, sounds, "C'est sur le plateau verdoyant de la région des lagunes,$dans la petite bourgade de Cocody rattaché à Abidjan que$naquit Zongo le Dozo en 1980. Véritable force de la nature,$nourri au terroir tumultueux de cette région sauvage$pendant 3 années, il se voit forcer de quitter la terre$originelle des Hommes pour venir dans l'enfer de la banlieue$parisienne.");
    textIntro(renderer, in, pictures, fonts, sounds, "Fort d'un baccalauréat STT, poursuivant des études$scientifiques à la factulté du val d'Oise, il apprend$l'anatomie pour parfaire en secret des techniques de$neutralisation léthales.");
    textIntro(renderer, in, pictures, fonts, sounds, "Gladiateur des temps modernes, montagne de testosterone,$il n'a fallu que quelques années pour forger la réputation$de cet être surhumain qui a gagné à la loterie génétique.$Zongo presse le pas, et commence à s'interroger sur son$avenir : s'il ne pouvait pas gravir les échelons de la$vieille Europe en plein émoi, il allait s'autoproclamer Roi,$et revétir un nouveau blason qui enverrait un message fort de$puissance. Il optera ainsi pour le pseudonyme \"Kaaris\" qui$signifie \"Roi des hommes\" en Ivoirien.");
    textIntro(renderer, in, pictures, fonts, sounds, "Invaincu en 78 combats menés clandestinement pour lesquels$il ne laisse aucun trace numérique par fierté, il décide à$contrecoeur d'accepter la proposition d'une exhibition$martial d'un troubadour local. Et c'est en Avril qu'il$revelera au monde sa vraie nature, à coup de salades de$phalanges savaemment placé par un tour de bras de 98 cm.$D'aucuns en ce monde n'est prêt pour tel spectacle.");
    textIntro(renderer, in, pictures, fonts, sounds, "Zongo le Dozo va réaliser son rêve, et son ennemi n'aura$le choix que de gouter à la subtille nuance salé de son$propre sang parfumé par la rage de Cocody.");
}





void textIntro(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, char *str)
{
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WINDOW_W, WINDOW_H);
    SDL_Rect pos_dst;
    unsigned long time1 = 0, time2 = 0;
    int escape = 0;
    int i = 0;
    int len = strlen(str);

    pos_dst.x = BORDER;
    pos_dst.y = 180;

    SDL_SetRenderTarget(renderer, texture);
    SDL_RenderCopy(renderer, pictures->title, NULL, NULL);
    SDL_SetRenderTarget(renderer, NULL);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ESCAPE || KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller.buttons[0] = 0;
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller.buttons[6] = 0;

            if(i < len)
            {
                while(i < len)
                {
                    printLetterOnTexture(renderer, texture, &pos_dst, fonts, str[i]);
                    i++;

                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, NULL, NULL);
                    SDL_RenderPresent(renderer);
                }
            }
            else
                escape = 1;
        }

        wait(&time1, &time2, DELAY_TEXT);


        if(i < len)
        {
            printLetterOnTexture(renderer, texture, &pos_dst, fonts, str[i]);
            i++;
            if(i % 4 == 0)
                Mix_PlayChannel(-1, sounds->text, 0);
        }


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

}



void printLetterOnTexture(SDL_Renderer *renderer, SDL_Texture *texture, SDL_Rect *pos_dst, Fonts *fonts, char c)
{
    SDL_Color color = {255, 255, 255};
    char character[2];
    character[0] = c;
    character[1] = '\0';

    SDL_Texture *letter = RenderTextBlended(renderer, fonts->ocraext_commands, character, color);
    SDL_QueryTexture(letter, NULL, NULL, &pos_dst->w, &pos_dst->h);

    if(c != '$')
    {
        SDL_SetRenderTarget(renderer, texture);
        SDL_RenderCopy(renderer, letter, NULL, pos_dst);
        pos_dst->x += pos_dst->w;
        SDL_SetRenderTarget(renderer, NULL);
    }
    else
    {
        pos_dst->y += pos_dst->h * 2;
        pos_dst->x = BORDER;
    }

    SDL_DestroyTexture(letter);
}

