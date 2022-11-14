#ifndef VIDEO_H
#define VIDEO_H

    #include "data.h"

    #define DELAY_VIDEO     1000 / 30

    int playVideo(SDL_Window *window, SDL_Renderer *renderer, Input *in, Fonts *fonts, char *filename);

#endif // VIDEO_H