#ifndef DATA_H
#define DATA_H

    #include <stdio.h>
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_ttf.h>
    #include <SDL2/SDL_mixer.h>
    #include <SDL2/SDL2_framerate.h>
    #if VERSION_INSTALL
        #ifdef __WIN64__
            #include <windows.h>
        #endif
    #endif // VERSION_INSTALL
    #include "event.h"

    #define NUM_SCORES      10
    #define NAME_LEN        11
    #define NUM_TIMES       (5 * NUM_LEVEL)
    #define NICKNAME_LEN    17

    typedef struct
    {
        TTF_Font *preview_title;
        TTF_Font *preview_intro;
        TTF_Font *ocraext_score;
        TTF_Font *ocraext_message;
        TTF_Font *ocraext_commands;
        TTF_Font *ocraext_editorHUD;
        TTF_Font *ocraext_version;
    } Fonts;

    typedef struct
    {
        SDL_Texture *HUDlife;
        SDL_Texture *HUDcoin;
        SDL_Texture *HUDtimer;
        SDL_Texture *title;
        SDL_Texture *explosion;
        SDL_Texture *highligthed;
        SDL_Texture *bullet_left;
        SDL_Texture *bullet_right;
        SDL_Texture *boss;
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
        Mix_Chunk *invicible;
        Mix_Chunk *complete;
        Mix_Chunk *select;
        Mix_Chunk *enter;
        Mix_Chunk *gun;
        Mix_Chunk *linefeed;
    } Sounds;

    typedef struct
    {
        int left;
        int right;
        int jump;
        int power_up;
    } Controls;

    typedef struct
    {
        int fullscreen;
        int music_volume;
        int sfx_volume;
        int haptic;
        Controls *controls;
        char nickname[NICKNAME_LEN];
    } Settings;


    Fonts* loadFonts();
    SDL_Texture* RenderTextBlended(SDL_Renderer *renderer, TTF_Font *font, char *str, SDL_Color fg);
    void BlitRenderTextBlended(SDL_Renderer *renderer, TTF_Font *font, char *str, SDL_Color fg, const int x, const int y);
    Pictures* loadPictures(SDL_Renderer *renderer);
    Sounds* loadSounds();
    Settings* loadSettings();
    Controls* loadControls();
    void getDefaultControls(Controls *controls);
    void saveSettings(Settings *settings);
    void saveControls(Controls *controls);
    void loadScores(unsigned long scores[], char names[][NAME_LEN]);
    void saveScores(unsigned long scores[], char names[][NAME_LEN]);
    void displayScoreList(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, unsigned long scores[], char names[][NAME_LEN], FPSmanager *fps);
    void enterName(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, char str[], int len, FPSmanager *fps);
    void loadTimes(unsigned long times[]);
    void saveTimes(unsigned long times[]);
    void updateTimes(const int level_num, const unsigned long player_time);

    #if VERSION_INSTALL
        #ifdef __WIN64__
            BOOL DirectoryExists(PWSTR dirName);
            BOOL FileExists(PWSTR szPath);
            FILE* getLocalAppdataFile(PWSTR filename, PWSTR mode);
        #endif // __WIN64__
    #endif // VERSION_INSTALL
#endif // DATA_H
