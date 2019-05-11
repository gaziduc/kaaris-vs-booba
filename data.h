#ifndef DATA_H
#define DATA_H

    #include <SDL2/SDL.h>
    #include <SDL2/SDL_ttf.h>
    #include <SDL2/SDL_mixer.h>
    #include "event.h"

    #define NUM_SCORES      10
    #define NAME_LEN        11
    #define NUM_TIMES       (5 * NUM_LEVEL)

    typedef struct
    {
        TTF_Font *ocraext_title;
        TTF_Font *ocraext_score;
        TTF_Font *ocraext_message;
        TTF_Font *ocraext_commands;
        TTF_Font *ocraext_editorHUD;
    } Fonts;

    typedef struct
    {
        SDL_Texture *HUDlife;
        SDL_Texture *HUDcoin;
        SDL_Texture *HUDtimer;
        SDL_Texture *gameover;
        SDL_Texture *title;
        SDL_Texture *explosion;
        SDL_Texture *highligthed;
    } Pictures;

    typedef struct
    {
        Mix_Chunk *death;
        Mix_Chunk *bumper;
        Mix_Chunk *jump;
        Mix_Chunk *life;
        Mix_Chunk *coin;
        Mix_Chunk *explosion;
        Mix_Chunk *checkpoint;
        Mix_Chunk *text;
        Mix_Chunk *invicible;
        Mix_Chunk *complete;
    } Sounds;

    typedef struct
    {
        int left;
        int right;
        int jump;
    } Controls;

    typedef struct
    {
        int fullscreen;
        int music_volume;
        int sfx_volume;
        Controls *controls;
    } Settings;


    Fonts* loadFonts();
    SDL_Texture* RenderTextBlended(SDL_Renderer *renderer, TTF_Font *font, char *str, SDL_Color fg);
    void BlitRenderTextBlended(SDL_Renderer *renderer, TTF_Font *font, char *str, SDL_Color fg, const int x, const int y);
    Pictures* loadPictures(SDL_Renderer *renderer);
    Sounds* loadSounds();
    Settings* loadSettings();
    Controls* loadControls();
    void saveSettings(Settings *settings);
    void saveControls(Controls *controls);
    void loadScores(unsigned long scores[], char names[][NAME_LEN]);
    void saveScores(unsigned long scores[], char names[][NAME_LEN]);
    void displayScoreList(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, unsigned long scores[], char names[][NAME_LEN]);
    void enterName(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, char str[]);
    void loadTimes(unsigned long times[]);
    void saveTimes(unsigned long times[]);
    void updateTimes(const int level_num, const unsigned long player_time);

#endif // DATA_H
