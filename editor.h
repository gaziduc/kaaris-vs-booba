#ifndef EDITOR_H
#define EDITOR_H

    void editor(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, FPSmanager *fps);
    void saveMap(Lvl *lvl);
    void displaySelectedTile(SDL_Renderer *renderer, Lvl *lvl, Input *in, int selected_tile);
    void displayEditorHUD(SDL_Renderer *renderer, Fonts *fonts, Lvl *lvl);
    void displayHighligth(SDL_Renderer *renderer, Input *in, Lvl *lvl);

#endif // EDITOR_H
