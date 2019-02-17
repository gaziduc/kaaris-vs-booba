#ifndef EDITOR_H
#define EDITOR_H

    void editor(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Settings *settings);
    void saveMap(Lvl *lvl);
    void displaySelectedTile(SDL_Renderer *renderer, Lvl *lvl, Input *in, Pictures *pictures, int selected_tile);
    void displayEditorHUD(SDL_Renderer *renderer, Fonts *fonts);
    void displayHighligth(SDL_Renderer *renderer, Input *in, Lvl *lvl);

#endif // EDITOR_H
