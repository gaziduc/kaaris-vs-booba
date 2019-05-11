#ifndef FILE_H
#define FILE_H

    #include "net.h"
    #include "data.h"

    enum {ALL_LEVELS, ONE_LEVEL, MODES_NUM};
    enum {SOLO_MODE, MULTI_MODE};

    void selectMode(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, const int num_player, Net *net);

#endif // FILE_H
