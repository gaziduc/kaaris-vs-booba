#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_framerate.h>
#include "data.h"
#include "game.h"
#include "transition.h"
#include "version.h"
#include "utils.h"

Fonts* loadFonts(SDL_Window *window)
{
    Fonts *fonts = xmalloc(sizeof(Fonts), window);

    fonts->preview_title = TTF_OpenFont("data/fonts/preview.otf", 65);
    fonts->preview_intro = TTF_OpenFont("data/fonts/preview.otf", 25);
    fonts->ocraext_score = TTF_OpenFont("data/fonts/ocraext.ttf", 35);
    fonts->ocraext_message = TTF_OpenFont("data/fonts/ocraext.ttf", 25);
    fonts->ocraext_commands = TTF_OpenFont("data/fonts/ocraext.ttf", 20);
    fonts->ocraext_editorHUD = TTF_OpenFont("data/fonts/ocraext.ttf", 16);
    fonts->ocraext_version = TTF_OpenFont("data/fonts/ocraext.ttf", 14);

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


Pictures* loadPictures(SDL_Renderer *renderer, SDL_Window *window)
{
    Pictures *pictures = xmalloc(sizeof(Pictures), window);

    pictures->HUDlife = IMG_LoadTexture(renderer, "data/gfx/life.bmp");
    pictures->HUDcoin = IMG_LoadTexture(renderer, "data/gfx/coin.png");
    pictures->HUDtimer = IMG_LoadTexture(renderer, "data/gfx/timer.png");
    pictures->title = IMG_LoadTexture(renderer, "data/gfx/title.jpg");
    pictures->explosion = IMG_LoadTexture(renderer, "data/gfx/explosion.png");
    pictures->bullet_left = IMG_LoadTexture(renderer, "data/gfx/bulletleft.bmp");
    pictures->bullet_right = IMG_LoadTexture(renderer, "data/gfx/bulletright.bmp");
    pictures->boss = IMG_LoadTexture(renderer, "data/gfx/booba.bmp");

    return pictures;
}


Sounds* loadSounds(SDL_Window *window)
{
    Sounds *sounds = xmalloc(sizeof(Sounds), window);

    sounds->death = Mix_LoadWAV("data/sfx/death.wav");
    sounds->bumper = Mix_LoadWAV("data/sfx/bumper.wav");
    sounds->jump = Mix_LoadWAV("data/sfx/jump.wav");
    sounds->life = Mix_LoadWAV("data/sfx/life.wav");
    sounds->coin = Mix_LoadWAV("data/sfx/coin.wav");
    sounds->explosion = Mix_LoadWAV("data/sfx/explosion.wav");
    sounds->checkpoint = Mix_LoadWAV("data/sfx/checkpoint.wav");
    sounds->invicible = Mix_LoadWAV("data/sfx/invicible.wav");
    sounds->complete = Mix_LoadWAV("data/sfx/complete.wav");
    sounds->select = Mix_LoadWAV("data/sfx/select.wav");
    sounds->enter = Mix_LoadWAV("data/sfx/enter.wav");
    sounds->gun = Mix_LoadWAV("data/sfx/gun.wav");
    sounds->linefeed = Mix_LoadWAV("data/sfx/linefeed.wav");

    return sounds;
}


Settings* loadSettings(SDL_Window *window)
{
    Settings *settings = xmalloc(sizeof(Settings), window);

    FILE *file = fopen("settings.ini", "r");

    if(file == NULL)
    {
        settings->fullscreen = 0;
        settings->music_volume = 128;
        settings->sfx_volume = 128;
        settings->haptic = 1;
        sprintf(settings->nickname, "Player %d", rand() % 90000 + 10000);

        saveSettings(settings);
    }
    else
    {
        fscanf(file, "Fullscreen=%d\nMusic Volume=%d\nSFX Volume=%d\nHaptic=%d\nNickname=", &settings->fullscreen, &settings->music_volume, &settings->sfx_volume, &settings->haptic);
        fgets(settings->nickname, sizeof(settings->nickname) / sizeof(settings->nickname[0]), file);

        char *end = strchr(settings->nickname, '\r');
        if(end != NULL)
            *end = '\0';

        end = strchr(settings->nickname, '\n');
        if(end != NULL)
            *end = '\0';

        fclose(file);
    }

    settings->controls = loadControls(window);

    return settings;
}



void saveSettings(Settings *settings)
{
    FILE *file = fopen("settings.ini", "w");

    if(file == NULL)
        exit(EXIT_FAILURE);

    fprintf(file, "Fullscreen=%d\nMusic Volume=%d\nSFX Volume=%d\nHaptic=%d\nNickname=%s\n", settings->fullscreen, settings->music_volume, settings->sfx_volume, settings->haptic, settings->nickname);
    fclose(file);
}






void loadScores(unsigned long scores[], char names[][NAME_LEN])
{
    FILE *file = fopen("scores.bin", "rb");

    if(file == NULL)
    {
        for(int i = 0; i < NUM_SCORES; i++)
        {
            scores[i] = 0;
            names[i][0] = '\0';
        }
    }
    else
    {
        for(int i = 0; i < NUM_SCORES; i++)
        {
            fread(&scores[i], sizeof(scores[i]), 1, file);
            fread(names[i], sizeof(names[i]), 1, file);
        }

        fclose(file);
    }
}




void saveScores(unsigned long scores[], char names[][NAME_LEN])
{
    FILE *file = fopen("scores.bin", "wb");

    if(file == NULL)
        exit(EXIT_FAILURE);

    for(int i = 0; i < NUM_SCORES; i++)
    {
        fwrite(&scores[i], sizeof(scores[i]), 1, file);
        fwrite(names[i], sizeof(names[i]), 1, file);
    }


    fclose(file);
}


void loadTimes(unsigned long times[])
{
    FILE *file = fopen("times.bin", "rb");

    if(file == NULL)
    {
        for(int i = 0; i < NUM_TIMES; i++)
            times[i] = 0;
    }
    else
    {
        for(int i = 0; i < NUM_TIMES; i++)
            fread(&times[i], sizeof(times[i]), 1, file);

        fclose(file);
    }
}

void saveTimes(unsigned long times[])
{
    FILE *file = fopen("times.bin", "wb");

    if(file == NULL)
        exit(EXIT_FAILURE);

    for(int i = 0; i < NUM_TIMES; i++)
        fwrite(&times[i], sizeof(times[i]), 1, file);

    fclose(file);
}


Controls* loadControls(SDL_Window *window)
{
    Controls *controls = xmalloc(sizeof(Controls) * 2, window);

    FILE *file = fopen("controls.ini", "r");

    if(file == NULL)
    {
        getDefaultControls(controls);
        saveControls(controls);
    }
    else
    {
        fscanf(file, "[PLAYER 1]\nLeft=%d\nRight=%d\nJump=%d\nPower up=%d\n\n[PLAYER 2]\nLeft=%d\nRight=%d\nJump=%d\nPower up=%d\n", &controls[0].left, &controls[0].right, &controls[0].jump, &controls[0].power_up, &controls[1].left, &controls[1].right, &controls[1].jump, &controls[1].power_up);
        fclose(file);
    }

    return controls;
}



void getDefaultControls(Controls *controls)
{
    controls[0].left = SDL_SCANCODE_A;
    controls[0].right = SDL_SCANCODE_D;
    controls[0].jump = SDL_SCANCODE_W;
    controls[0].power_up = SDL_SCANCODE_X;

    controls[1].left = SDL_SCANCODE_LEFT;
    controls[1].right = SDL_SCANCODE_RIGHT;
    controls[1].jump = SDL_SCANCODE_UP;
    controls[1].power_up = SDL_SCANCODE_SPACE;
}


void saveControls(Controls *controls)
{
    FILE *file = fopen("controls.ini", "w");

    if(file == NULL)
        exit(EXIT_FAILURE);

    fprintf(file, "[PLAYER 1]\nLeft=%d\nRight=%d\nJump=%d\nPower up=%d\n\n[PLAYER 2]\nLeft=%d\nRight=%d\nJump=%d\nPower up=%d\n", controls[0].left, controls[0].right, controls[0].jump, controls[0].power_up, controls[1].left, controls[1].right, controls[1].jump, controls[1].power_up);
    fclose(file);
}




void displayScoreList(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, unsigned long scores[], char names[][NAME_LEN], FPSmanager *fps)
{
    SDL_Color white = {255, 255, 255, 255};
    SDL_Texture *texture[NUM_SCORES + 1];
    SDL_Rect pos_dst[NUM_SCORES + 1];
    char str[100] = "";

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


    transition(renderer, pictures->title, 11, texture, pos_dst, ENTERING, 1, fps);
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
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[1].buttons[6] = 0;
            escape = 1;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = 0; i < 11; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);
    }

    transition(renderer, pictures->title, 11, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < NUM_SCORES + 1; i++)
        SDL_DestroyTexture(texture[i]);
}




void enterName(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, char str[], int len, FPSmanager *fps)
{
    SDL_Color white = {255, 255, 255, 255};
    int escape = 0;
    int frame = 0;
    SDL_Texture *texture[3];
    SDL_Rect pos_dst[3];
    SDL_Rect pos_fs;
    pos_fs.x = 0;
    pos_fs.y = 0;
    pos_fs.w = WINDOW_W;
    pos_fs.h = WINDOW_H;

    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, "Entrez votre nom :", white);
    SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
    pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;
    pos_dst[0].y = WINDOW_H / 2 - 50 - pos_dst[0].h / 2;


    texture[1] = RenderTextBlended(renderer, fonts->ocraext_score, "|", white);
    SDL_QueryTexture(texture[1], NULL, NULL, &pos_dst[1].w, &pos_dst[1].h);
    pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
    pos_dst[1].y = WINDOW_H / 2 + 50 - pos_dst[1].h / 2 + 2;


    transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 1, fps);


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


        strncat(str, in->text, len - strlen(str) - 1);

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
        SDL_RenderCopy(renderer, pictures->title, NULL, &pos_fs);

        for(int i = 0; i < 3; i++)
            if(i == 0 || (i == 2 && str[0] != '\0') || (i == 1 && frame < 30))
                SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        if(str[0] != '\0')
            SDL_DestroyTexture(texture[2]);

        frame++;
        frame = frame % 60;
    }

    SDL_StopTextInput();

    texture[2] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
    SDL_QueryTexture(texture[2], NULL, NULL, &pos_dst[2].w, &pos_dst[2].h);
    pos_dst[2].x = WINDOW_W / 2 - pos_dst[2].w / 2;
    pos_dst[2].y = WINDOW_H / 2 + 50 - pos_dst[2].h / 2;

    transition(renderer, pictures->title, 3, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < 3; i++)
        SDL_DestroyTexture(texture[i]);
}



void updateTimes(const int level_num, const unsigned long player_time)
{
    unsigned long times[NUM_TIMES];
    loadTimes(times);

    int i = level_num * 5 - 1;
    while(i >= (level_num - 1) * 5 && (player_time < times[i] || times[i] == 0))
    {
        if(i == level_num * 5 - 1)
            times[i] = player_time;
        else
        {
            unsigned long time = times[i];
            times[i] = player_time;
            times[i + 1] = time;
        }

        i--;
    }

    saveTimes(times);
}
