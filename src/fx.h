#ifndef FX_H
#define FX_H

    #include <SDL2/SDL.h>
    #include "game.h"

    /* Interrupteurs : mettre 0 pour desactiver un effet */
    #define FX_SHAKE     1
    #define FX_VIGNETTE  1

    /* Effets visuels : particules (coordonnees monde), screen shake, vignette, squash & stretch. */

    void fxInit(SDL_Renderer *renderer);
    void fxQuit(void);
    void fxClear(void);
    void fxUpdate(void);

    void fxDust(float x, float y, int dir, int n);
    void fxBurst(float x, float y, int n, int r, int g, int b);
    void fxSparkle(float x, float y, int n);
    void fxHearts(float x, float y);
    void fxRing(float x, float y, int r, int g, int b);
    void fxFlame(float x, float y);
    void fxShake(float amount);

    /* A appeler une fois par image et par joueur : detecte saut, atterrissage, pieces, mort... */
    void fxPlayerEvents(int idx, Player *player);
    void fxPlayerScale(int idx, float *sx, float *sy);
    void fxShadow(SDL_Renderer *renderer, Lvl *lvl, Player *player, int view, int num_players);

    void fxDraw(SDL_Renderer *renderer, int startX, int startY, int view_y, int view_h);
    void fxVignette(SDL_Renderer *renderer);
    void fxShakeBegin(Lvl *lvl, int num_players);
    void fxShakeEnd(Lvl *lvl, int num_players);

#endif // FX_H
