#ifndef TRANSITION_H
#define TRANSITION_H

    #include "data.h"

    #define MAX_SPEED   102

    enum {ENTERING, EXITING};

    void transition(SDL_Renderer *renderer, SDL_Texture *bg, int num_textures, SDL_Texture *texture[], SDL_Rect pos_dst[], const int type, const int number);
    void levelFinished(SDL_Renderer *renderer, Sounds *sounds, Fonts *fonts, SDL_Texture *texture);

#endif // TRANSITION_H
