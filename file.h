#ifndef FILE_H
#define FILE_H

    #include <SDL2/SDL2_framerate.h>
    #include "net.h"
    #include "data.h"

    enum {ALL_LEVELS, ONE_LEVEL, SCORES, MODES_NUM};

    void selectMode(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, const int num_player, Net *net, FPSmanager *fps);

#endif // FILE_H
