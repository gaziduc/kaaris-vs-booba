#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>


void *xmalloc(size_t size, SDL_Window *window)
{
    void *ptr = malloc(size);

    if (!ptr)
    {
        SDL_DestroyWindow(window);
        char s[100] = { 0 };
        sprintf(s, "Allocation of %zu bytes in memory failed.", size);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Memory allocation failed", s, NULL);
        exit(EXIT_FAILURE);
    }

    return ptr;
}
