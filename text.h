#ifndef TEXT_H
#define TEXT_H

    void intro(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, FPSmanager *fps);
    int textIntro(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, FPSmanager *fps, char *str);

#endif // TEXT_H
