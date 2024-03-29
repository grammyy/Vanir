#ifndef WINDOWS
#define WINDOWS

#include <SDL.h>
#include <stdbool.h>

int windowsInit(lua_State* L);

struct windowPool {
    struct sdlWindow **windows;
    int count;
};

struct sdlWindow {
    int x, y, width, height, ref;
    const char *name;
    bool quit, hovering, focused;
    SDL_Window* window;
    SDL_GLContext context;
    Uint32 id;
};

#endif