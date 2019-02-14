#ifndef TRANSITION_H
#define TRANSITION_H

    #define MAX_SPEED   102

    enum {ENTERING, EXITING};

    void transition(SDL_Renderer *renderer, SDL_Texture *bg, int num_textures, SDL_Texture *texture[], SDL_Rect pos_dst[], const int type, const int number);

#endif // TRANSITION_H
