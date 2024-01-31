#ifndef WINDOWS_H
#define WINDOWS_H

#include <lua.h>
#include <SDL.h>

int windowsInit(lua_State* L);

struct windowPool {
    struct sdlWindow *windows;
    int count;
};

struct sdlWindow {
    int x, y, width, height, quit;
    const char *name;
    SDL_Window* window;
    SDL_GLContext context;
    Uint32 id;
};

extern struct windowPool windowPool;

extern struct hook render;

#endif