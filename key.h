#ifndef KEY_H
#define KEY_H

    enum {HEADER, GO_LEFT, GO_RIGHT, JUMP, NUM_KEYS};

    void displayKeys(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, Settings *settings);
    void changeKey(SDL_Renderer *renderer, SDL_Texture *texture, Settings *settings, Fonts *fonts, int player_num, int key);

#endif // KEY_H
