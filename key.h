#ifndef KEY_H
#define KEY_H

    enum {HEADER, VLINE0, HLINE, VLINE1, GO_LEFT, VLINE2, GO_RIGHT, VLINE3, JUMP, VLINE4, BACK, VLINE5, NUM_KEYS};

    void displayKeys(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in);

#endif // KEY_H
