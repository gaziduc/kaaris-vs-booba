#ifndef TEXT_H
#define TEXT_H

    #define BORDER      300
    #define DELAY_TEXT  1000 / 30

    void intro(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds);
    void textIntro(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, char *str);
    void printLetterOnTexture(SDL_Renderer *renderer, SDL_Texture *texture, SDL_Rect *pos_dst, Fonts *fonts, char c);


#endif // TEXT_H
