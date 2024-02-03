#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include "../vanir.h"
#include "windows.h"
#include "hooks.h"
#include "render.h"

struct windowPool windowPool = {NULL, 0};

void renderHandle(struct hook *instance, lua_State *L) {
    if (windowPool.count==0) {
        instance->status=hook_idle;
    } else {
        instance->status=hook_update;
    }
}

struct hook render = {"render", NULL, 0, &render, renderHandle, hook_update};

// window methods ↓↓↓ window methods ///
int isHovering(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");
    
    lua_pushboolean(L, (*window)->hovering);

    return 1;
}

int isFocused(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");
    
    lua_pushboolean(L, (*window)->focused);

    return 1;
}

int getTitle(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");

    lua_pushstring(L, (*window)->name);

    return 1;
}

int getID(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");
    
    lua_pushinteger(L, (*window)->id);

    return 1;
}
// window methods ↑↑↑ window methods ///

void* newWindow(void* data) {
    struct sdlWindow* window = (struct sdlWindow*)data;
    
    window->window = SDL_CreateWindow(
        window->name,
        window->x ? window->x : SDL_WINDOWPOS_UNDEFINED,
        window->y ? window->y : SDL_WINDOWPOS_UNDEFINED,
        window->width,
        window->height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
    );

    window->id=SDL_GetWindowID(window->window);

    if (!window->window) {
        throw("Window", window->name, SDL_GetError());

        SDL_Quit();
        
        return NULL;
    }

    window->context = SDL_GL_CreateContext(window->window);

    if (!window->context) {
        throw("Window", window->name, "Failed to create OpenGL context");

        SDL_DestroyWindow(window->window);
        SDL_Quit();
        
        return NULL;
    }

    struct sdlWindow* temp = (struct sdlWindow*)realloc(windowPool.windows, (windowPool.count + 1) * sizeof(struct sdlWindow));

    if (temp != NULL) {
        windowPool.windows = temp;
        windowPool.windows[windowPool.count] = *window;
        windowPool.count += 1;
    } else {
        throw("Window", window->name, "Memory allocation error");
    }

    while (!window->quit) {
        SDL_GL_MakeCurrent(window->window, window->context);

        SDL_Event event;
        
        if (SDL_PollEvent(&event)) {
            while (event.window.windowID != window->id) {
                if (!SDL_WaitEvent(&event)) {
                    throw("Window", window->name, SDL_GetError());

                    return NULL;
                }
            }
        }

        if (event.type == SDL_WINDOWEVENT) {
            switch(event.window.event){
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    int newWidth = event.window.data1;
                    int newHeight = event.window.data2;

                    glViewport(0, 0, newWidth, newHeight);

                    //SDL_GL_SwapWindow(window->window);

                    break;
                case SDL_WINDOWEVENT_CLOSE:
                    window->quit = true;

                    break;
                case SDL_WINDOWEVENT_ENTER:
                    window->hovering = true;
                    
                    break;
                case SDL_WINDOWEVENT_LEAVE:
                    window->hovering = false;
                    
                    break;
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    window->focused = true;

                    break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    window->focused = false;

                    break;
            }
        }

        SDL_GL_MakeCurrent(NULL, NULL);
    }

    for (int i = 0; i < windowPool.count; ++i) {
        if (windowPool.windows[i].id == window->id) {
            for (int j = i; j < windowPool.count - 1; ++j) {
                windowPool.windows[j] = windowPool.windows[j + 1];
            }
            windowPool.count -= 1;
            
            break;
        }
    }

    window = {};

    SDL_DestroyWindow(window->window);
    SDL_GL_DeleteContext(window->context);
    SDL_FlushEvent(SDL_WINDOWEVENT);

    free(window);

    return NULL;
}

int createWindow(lua_State *L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int width = luaL_checkinteger(L, 3);
    int height = luaL_checkinteger(L, 4);
    const char *name = luaL_checkstring(L, 5);

    struct sdlWindow *window = malloc(sizeof(struct sdlWindow));

    if (!window) {
        fprintf(stderr, "Memory allocation for window failed\n");

        throw("Window", name, "Memory allocation error");

        return 0;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->name = name;
    window->quit=0;

    pthread_t glThread;

    if (pthread_create(&glThread, NULL, newWindow, (void *)window) != 0) {
        throw("Window", name, "OpenGL thread error");
        
        free(window);
    }

    struct sdlWindow **udata = (struct sdlWindow **)lua_newuserdata(L, sizeof(struct sdlWindow *));
    *udata = window;

    luaL_newmetatable(L, "window");
    
    lua_pushstring(L, "__index"); //use function defined methods later
    lua_newtable(L);
    lua_pushcfunction(L, selectRender);
    lua_setfield(L, -2, "selectRender");
    lua_pushcfunction(L, stopRender);
    lua_setfield(L, -2, "stopRender");
    lua_pushcfunction(L, update);
    lua_setfield(L, -2, "update");
    lua_pushcfunction(L, isHovering);
    lua_setfield(L, -2, "isHovering");
    lua_pushcfunction(L, isFocused);
    lua_setfield(L, -2, "isFocused");
    lua_pushcfunction(L, getTitle);
    lua_setfield(L, -2, "getTitle");
    lua_pushcfunction(L, getID);
    lua_setfield(L, -2, "getID");
    lua_settable(L, -3);

    lua_setmetatable(L, -2);

    return 1;
}

const luaL_Reg luaWindows[] = {
    {"createWindow", createWindow},
    {NULL, NULL}
};

int windowsInit(lua_State* L) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw("Init", "SDL", SDL_GetError());

        return 1;
    }
    
    luaL_newlib(L, luaWindows);

    registerHook(&pool, render);

    return 1;
}