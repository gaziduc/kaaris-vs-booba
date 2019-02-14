#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "data.h"
#include "game.h"
#include "transition.h"

Fonts* loadFonts()
{
    Fonts *fonts = malloc(sizeof(Fonts));
    if(fonts == NULL)
        exit(EXIT_FAILURE);

    fonts->ocraext_title = TTF_OpenFont("./data/fonts/ocraext.ttf", 60);
    fonts->ocraext_score = TTF_OpenFont("./data/fonts/ocraext.ttf", 35);
    fonts->ocraext_message = TTF_OpenFont("./data/fonts/ocraext.ttf", 25);
    fonts->ocraext_commands = TTF_OpenFont("./data/fonts/ocraext.ttf", 20);
    fonts->ocraext_editorHUD = TTF_OpenFont("./data/fonts/ocraext.ttf", 15);

    return fonts;
}


SDL_Texture* RenderTextBlended(SDL_Renderer *renderer, TTF_Font *font, char *str, SDL_Color fg)
{
    SDL_Surface *surface = TTF_RenderText_Blended(font, str, fg);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}

void BlitRenderTextBlended(SDL_Renderer *renderer, TTF_Font *font, char *str, SDL_Color fg, const int x, const int y)
{
    SDL_Texture *texture = RenderTextBlended(renderer, font, str, fg);

    SDL_Rect pos_dst;
    pos_dst.x = x;
    pos_dst.y = y;
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);

    SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
    SDL_DestroyTexture(texture);
}


Pictures* loadPictures(SDL_Renderer *renderer)
{
    Pictures *pictures = malloc(sizeof(Pictures));
    if(pictures == NULL)
        exit(EXIT_FAILURE);

    pictures->HUDlife = IMG_LoadTexture(renderer, "./data/hud/life.bmp");
    pictures->HUDcoin = IMG_LoadTexture(renderer, "./data/hud/coin.png");
    pictures->gameover = IMG_LoadTexture(renderer, "./data/background/gameover.png");
    pictures->title = IMG_LoadTexture(renderer, "./data/background/title.jpg");
    pictures->explosion = IMG_LoadTexture(renderer, "./data/tilesets/explosion.png");

    return pictures;
}


Sounds* loadSounds()
{
    Sounds *sounds = malloc(sizeof(Sounds));

    sounds->death = Mix_LoadWAV("./data/sfx/death.wav");
    sounds->bumper = Mix_LoadWAV("./data/sfx/bumper.wav");
    sounds->jump = Mix_LoadWAV("./data/sfx/jump.wav");
    sounds->life = Mix_LoadWAV("./data/sfx/life.wav");
    sounds->coin = Mix_LoadWAV("./data/sfx/coin.wav");
    sounds->explosion = Mix_LoadWAV("./data/sfx/explosion.wav");
    sounds->checkpoint = Mix_LoadWAV("./data/sfx/checkpoint.wav");
    sounds->text = Mix_LoadWAV("./data/sfx/text.wav");
    sounds->invicible = Mix_LoadWAV("./data/sfx/invicible.wav");

    return sounds;
}


Settings* loadSettings()
{
    Settings *settings = malloc(sizeof(Settings));
    if(settings == NULL)
        exit(EXIT_FAILURE);

    FILE *file = fopen("./start.ini", "r");
    if(file == NULL)
        exit(EXIT_FAILURE);

    fscanf(file, "Fullscreen=%d\n", &settings->fullscreen);

    fclose(file);

    return settings;
}



void saveSettings(Settings *settings)
{
    FILE *file = fopen("./start.ini", "w");
    if(file == NULL)
        exit(EXIT_FAILURE);

    fprintf(file, "Fullscreen=%d\n", settings->fullscreen);

    fclose(file);
}






void loadScores(unsigned long scores[], char names[][NAME_LEN])
{
    FILE *file = fopen("./scores.bin", "rb");
    if(file == NULL)
        exit(EXIT_FAILURE);

    for(int i = 0; i < NUM_SCORES; i++)
    {
        fread(&scores[i], sizeof(scores[i]), 1, file);
        fread(names[i], sizeof(names[i]), 1, file);
    }


    fclose(file);
}




void saveScores(unsigned long scores[], char names[][NAME_LEN])
{
    FILE *file = fopen("./scores.bin", "wb");
    if(file == NULL)
        exit(EXIT_FAILURE);

    for(int i = 0; i < NUM_SCORES; i++)
    {
        fwrite(&scores[i], sizeof(scores[i]), 1, file);
        fwrite(names[i], sizeof(names[i]), 1, file);
    }


    fclose(file);
}



void displayScoreList(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, unsigned long scores[], char names[][NAME_LEN])
{
    SDL_Color white = {255, 255, 255};
    SDL_Texture *texture[NUM_SCORES + 1];
    SDL_Rect pos_dst[NUM_SCORES + 1];
    char str[100] = "";
    unsigned long time1 = 0, time2 = 0;

    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, "Scores", white);
    SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
    pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;
    pos_dst[0].y = 85;

    for(int i = 1; i < NUM_SCORES + 1; i++)
    {
        sprintf(str, "%d)    %s    %ld", i, (names[i - 1][0] == '\0') ? "AAA" : names[i - 1], scores[i - 1]);
        texture[i] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        pos_dst[i].y = 100 + i * 50;
    }


    transition(renderer, pictures->title, 11, texture, pos_dst, ENTERING, 1);
    int escape = 0;

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ENTER_MENU || KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller.buttons[0] = 0;
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller.buttons[6] = 0;
            escape = 1;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = 0; i < 11; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);

        wait(&time1, &time2, DELAY_GAME);
    }

    transition(renderer, pictures->title, 11, texture, pos_dst, EXITING, 0);

    for(int i = 0; i < NUM_SCORES + 1; i++)
        SDL_DestroyTexture(texture[i]);
}




void enterName(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, char str[])
{
    SDL_Color white = {255, 255, 255};
    int escape = 0;
    int frame = 0;
    unsigned long time1 = 0, time2 = 0;
    SDL_Texture *texture[3];
    SDL_Rect pos_dst[3];

    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, "Entrez votre nom :", white);
    SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
    pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;
    pos_dst[0].y = WINDOW_H / 2 - 50 - pos_dst[0].h / 2;


    texture[1] = RenderTextBlended(renderer, fonts->ocraext_score, "|", white);
    SDL_QueryTexture(texture[1], NULL, NULL, &pos_dst[1].w, &pos_dst[1].h);
    pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
    pos_dst[1].y = WINDOW_H / 2 + 50 - pos_dst[1].h / 2 + 2;


    transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 1);


    SDL_StartTextInput();

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(in->key[SDL_SCANCODE_RETURN] || in->key[SDL_SCANCODE_KP_ENTER])
        {
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;

            if(str[0] != '\0')
                escape = 1;
        }
        if(in->key[SDL_SCANCODE_BACKSPACE])
        {
            in->key[SDL_SCANCODE_BACKSPACE] = 0;

            int i = 0;
            while(str[i] != '\0')
                i++;

            if(i > 0)
                str[i - 1] = '\0';
        }


        if(strlen(str) + strlen(in->text) + 1 <= NAME_LEN)
            strcat(str, in->text);

        if(str[0] != '\0')
        {
            texture[2] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
            SDL_QueryTexture(texture[2], NULL, NULL, &pos_dst[2].w, &pos_dst[2].h);
            pos_dst[2].x = WINDOW_W / 2 - pos_dst[2].w / 2;
            pos_dst[2].y = WINDOW_H / 2 + 50 - pos_dst[2].h / 2;

            pos_dst[1].x = WINDOW_W / 2 + pos_dst[2].w / 2 - pos_dst[1].w / 2 + 2;
        }
        else
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2 + 2;


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = 0; i < 3; i++)
            if(i == 0 || (i == 2 && str[0] != '\0') || (i == 1 && frame < 30))
                SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);

        if(str[0] != '\0')
            SDL_DestroyTexture(texture[2]);

        wait(&time1, &time2, DELAY_GAME);
        frame++;
        frame = frame % 60;
    }

    SDL_StopTextInput();

    texture[2] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
    SDL_QueryTexture(texture[2], NULL, NULL, &pos_dst[2].w, &pos_dst[2].h);
    pos_dst[2].x = WINDOW_W / 2 - pos_dst[2].w / 2;
    pos_dst[2].y = WINDOW_H / 2 + 50 - pos_dst[2].h / 2;

    transition(renderer, pictures->title, 3, texture, pos_dst, EXITING, 0);

    for(int i = 0; i < 3; i++)
        SDL_DestroyTexture(texture[i]);
}



