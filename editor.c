#include <stdio.h>
#include <SDL2/SDL.h>
#include "game.h"
#include "editor.h"

void editor(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts)
{
    int escape = 0;
    int selected_tile = 1;

    SDL_ShowCursor(SDL_ENABLE);
    Lvl *lvl = malloc(sizeof(Lvl));
    if(lvl == NULL)
        exit(EXIT_FAILURE);

    loadLevel(renderer, 1, lvl, EDIT);



    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller.buttons[6] = 0;
            escape = 1;
        }
        if(KEY_LEFT_GAME)
        {
            lvl->startX -= TILE_SIZE;
            if(lvl->startX < 0)
                lvl->startX = 0;
        }
        if(KEY_RIGHT_GAME)
        {
            lvl->startX += TILE_SIZE;
            if (lvl->startX + WINDOW_W >= lvl->maxX)
                lvl->startX = lvl->maxX - WINDOW_W;
        }
        if(KEY_UP_MENU)
        {
            lvl->startY -= TILE_SIZE;
            if(lvl->startY < 0)
                lvl->startY = 0;
        }
        if(KEY_DOWN_MENU)
        {
            lvl->startY += TILE_SIZE;
            if (lvl->startY + WINDOW_H >= lvl->maxY)
                lvl->startY = lvl->maxY - WINDOW_H;
        }
        if(in->mousebutton[SDL_BUTTON_LEFT])
            lvl->map[(lvl->startX + in->mouseX) / TILE_SIZE][(lvl->startY + in->mouseY) / TILE_SIZE] = selected_tile;
        if(in->mousebutton[SDL_BUTTON_RIGHT])
            lvl->map[(lvl->startX + in->mouseX) / TILE_SIZE][(lvl->startY + in->mouseY) / TILE_SIZE] = 0;
        if(in->wheelY > 0)
        {
            selected_tile++;
            if(selected_tile >= lvl->num_tiles)
                selected_tile = 0;
        }
        if(in->wheelY < 0)
        {
            selected_tile--;
            if(selected_tile < 0)
                selected_tile = lvl->num_tiles - 1;
        }
        if(in->key[SDL_SCANCODE_S])
        {
            in->key[SDL_SCANCODE_S] = 0;
            saveMap(lvl);
        }
        if(in->key[SDL_SCANCODE_PAGEUP])
        {
            in->key[SDL_SCANCODE_PAGEUP] = 0;

            if(lvl->number + 1 <= NUM_LEVEL)
            {
                freeLevel(lvl, EDIT);
                loadLevel(renderer, lvl->number + 1, lvl, EDIT);
            }
        }
        if(in->key[SDL_SCANCODE_PAGEDOWN])
        {
            in->key[SDL_SCANCODE_PAGEDOWN] = 0;

            if(lvl->number - 1 > 0)
            {
                freeLevel(lvl, EDIT);
                loadLevel(renderer, lvl->number - 1, lvl, EDIT);
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, lvl->sky, NULL, NULL);
        displayGame(renderer, pictures, lvl, NULL, 0, EDIT);
        displayHighligth(renderer, in, lvl);
        displayEditorHUD(renderer, fonts);
        displaySelectedTile(renderer, lvl, in, pictures, selected_tile);
        SDL_RenderPresent(renderer);

        if(KEY_LEFT_GAME || KEY_RIGHT_GAME || KEY_DOWN_MENU || KEY_UP_MENU)
            SDL_Delay(30);
    }


    freeLevel(lvl, EDIT);
    free(lvl);

    SDL_ShowCursor(SDL_DISABLE);
}


void displaySelectedTile(SDL_Renderer *renderer, Lvl *lvl, Input *in, Pictures *pictures, int selected_tile)
{
    SDL_Texture *selected_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, TILE_SIZE, TILE_SIZE);
    SDL_SetTextureBlendMode(selected_texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, selected_texture);

    SDL_Rect pos_src;
    pos_src.x = selected_tile * TILE_SIZE;
    pos_src.y = 0;
    pos_src.w = TILE_SIZE;
    pos_src.h = TILE_SIZE;

    SDL_RenderCopy(renderer, lvl->tileset, &pos_src, NULL);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, NULL);

    SDL_SetRenderTarget(renderer, NULL);

    SDL_Rect pos_dst;
    pos_dst.x = in->mouseX + TILE_SIZE / 2;
    pos_dst.y = in->mouseY + TILE_SIZE / 2;
    pos_dst.w = TILE_SIZE;
    pos_dst.h = TILE_SIZE;
    SDL_RenderCopy(renderer, selected_texture, NULL, &pos_dst);

    SDL_DestroyTexture(selected_texture);
}


void saveMap(Lvl *lvl)
{
    char str[200] = "";

    FILE *file = NULL;

    sprintf(str, "./data/maps/map_%d.txt", lvl->number);
    file = fopen(str, "r+");
    if(file == NULL)
        exit(EXIT_FAILURE);

    rewind(file);

    // Jump to line 13
    for(int i = 0; i < 12; i++)
        fgets(str, sizeof(str), file);

    for(int y = 0; y < lvl->height; y++)
    {
        for(int x = 0; x < lvl->width; x++)
        {
            fseek(file, 0, SEEK_CUR);
            fputc(lvl->map[x][y] + '0', file);
        }

        fseek(file, 0, SEEK_CUR);
        fputc('\n', file);
    }

    fclose(file);
}




void displayEditorHUD(SDL_Renderer *renderer, Fonts *fonts)
{
    SDL_Color color = {255, 0, 0};
    BlitRenderTextBlended(renderer, fonts->ocraext_editorHUD, "Se déplacer : touches fléchées", color, 15, 10);
    BlitRenderTextBlended(renderer, fonts->ocraext_editorHUD, "Changer tile : molette souris", color, 15, 30);
    BlitRenderTextBlended(renderer, fonts->ocraext_editorHUD, "Poser tile : clic gauche", color, 15, 50);
    BlitRenderTextBlended(renderer, fonts->ocraext_editorHUD, "Effacer tile : clic droit", color, 15, 70);
    BlitRenderTextBlended(renderer, fonts->ocraext_editorHUD, "Niveau suivant/précédent : Page up/down", color, 15, 90);
    BlitRenderTextBlended(renderer, fonts->ocraext_editorHUD, "Sauvegarder : S", color, 15, 110);
    BlitRenderTextBlended(renderer, fonts->ocraext_editorHUD, "Quitter : Echap", color, 15, 130);
}





void displayHighligth(SDL_Renderer *renderer, Input *in, Lvl *lvl)
{
    SDL_Texture *highligthed = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, TILE_SIZE, TILE_SIZE);
    SDL_SetTextureBlendMode(highligthed, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, highligthed);
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 128);
    SDL_RenderFillRect(renderer, NULL);
    SDL_SetRenderTarget(renderer, NULL);

    SDL_Rect pos_dst;
    pos_dst.x = ((in->mouseX  + lvl->startX % TILE_SIZE) / TILE_SIZE) * TILE_SIZE + (lvl->startX % TILE_SIZE) * -1;
    pos_dst.y = ((in->mouseY  + lvl->startY % TILE_SIZE) / TILE_SIZE) * TILE_SIZE + (lvl->startY % TILE_SIZE) * -1;
    pos_dst.w = TILE_SIZE;
    pos_dst.h = TILE_SIZE;
    SDL_RenderCopy(renderer, highligthed, NULL, &pos_dst);

    SDL_DestroyTexture(highligthed);
}

