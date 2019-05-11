#ifndef MULTI_H
#define MULTI_H

    enum {START_GAME, P1_TYPE, P2_TYPE, NUM_TEXT_MULTI};
    enum {KEYBOARD, CONTROLLER};

    void selectMultiCommandType(SDL_Renderer *renderer, Input *in, Fonts *fonts, Pictures *pictures, Sounds *sounds, Mix_Music **music, Settings *settings);

#endif // MULTI_H
