#ifndef TRANSITION_H
#define TRANSITION_H

    #include "data.h"
    #include "game.h"

    #define MAX_SPEED   105

    enum {ENTERING, EXITING};
    enum {PAUSE_TITLE, RESUME, QUIT_PAUSE, NUM_TEXT_PAUSE};

    void transition(SDL_Renderer *renderer, SDL_Texture *bg, int num_textures, SDL_Texture *texture[], SDL_Rect pos_dst[], const int type, const int number, FPSmanager *fps);
    void levelFinished(SDL_Renderer *renderer, Fonts *fonts, Player *player, int level_num, int num_player, int player_num, int lvl_finished[]);
    int pauseGame(SDL_Renderer *renderer, SDL_Texture *texture, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, FPSmanager *fps);

#endif // TRANSITION_H
