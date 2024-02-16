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

void glAspectRatio(int width, int height) {
    float aspectRatio = (float) width / (float) height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (aspectRatio >= 1.0) {
        glOrtho(-aspectRatio, aspectRatio, -1.0, 1.0, -1.0, 1.0);
    } else {
        float halfHeight = 1.0 / aspectRatio;

        glOrtho(-1.0, 1.0, -halfHeight, halfHeight, -1.0, 1.0);
    }

    glMatrixMode(GL_MODELVIEW);
}

void renderHandle(struct hook *instance, lua_State *L) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_WINDOWEVENT) {
            for (size_t i = 0; i < windowPool.count; ++i) {
                printf("%d %d\n",windowPool.windows[i]->id, event.window.windowID);

                if (windowPool.windows[i]->id == event.window.windowID) {
                    SDL_GL_MakeCurrent(windowPool.windows[i]->window, windowPool.windows[i]->context);

                    switch(event.window.event){
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            int width = event.window.data1;
                            int height = event.window.data2;

                            glViewport(0, 0, width, height);

                            glAspectRatio(width, height);

                            //SDL_GL_SwapWindow(window->window);

                            break;
                        case SDL_WINDOWEVENT_CLOSE:
                            windowPool.windows[i]->quit = true;
                            
                            SDL_DestroyWindow(windowPool.windows[i]->window);
                            SDL_GL_DeleteContext(windowPool.windows[i]->context);
                            
                            for (int j = i; j < windowPool.count - 1; ++j) {
                                windowPool.windows[j] = windowPool.windows[j + 1];
                            }
                            
                            windowPool.count -= 1;

                            break;
                        case SDL_WINDOWEVENT_ENTER:
                            windowPool.windows[i]->hovering = true;
                            
                            break;
                        case SDL_WINDOWEVENT_LEAVE:
                            windowPool.windows[i]->hovering = false;
                            
                            break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            windowPool.windows[i]->focused = true;

                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            windowPool.windows[i]->focused = false;

                            break;
                    }

                    SDL_GL_MakeCurrent(NULL, NULL);
                }
            }
        }
    }
    
    if (windowPool.count==0)
        instance->status=hook_idle;
    else
        instance->status=hook_update;
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

void newWindow(struct sdlWindow *window) {
    window->window = SDL_CreateWindow(
        window->name,
        window->x,
        window->y,
        window->width,
        window->height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
    );

    window->id=SDL_GetWindowID(window->window);

    if (!window->window) {
        throw("Window", window->name, SDL_GetError());

        SDL_Quit();
        
        return;
    }

    window->context = SDL_GL_CreateContext(window->window);

    if (!window->context) {
        throw("Window", window->name, "Failed to create OpenGL context");

        SDL_DestroyWindow(window->window);
        SDL_Quit();
        
        return;
    }

    struct sdlWindow **temp = (struct sdlWindow **)realloc(windowPool.windows, (windowPool.count + 1) * sizeof(struct sdlWindow *));

    if (temp != NULL) {
        windowPool.windows = temp;
        windowPool.windows[windowPool.count] = window; // Store the pointer directly
        windowPool.count += 1;
    } else {
        // Handle memory allocation error
        fprintf(stderr, "Window Memory allocation error\n");
    }
    
    SDL_GL_MakeCurrent(window->window, window->context);

    glAspectRatio(window->width, window->height);

    // Enable anti-aliasing for lines
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable anti-aliasing for polygons
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    // Enable anti-aliasing for textures
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);

    // Enable anti-aliasing for fragment shader derivatives
    glEnable(GL_FRAGMENT_SHADER_DERIVATIVE_HINT);
    glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);

    SDL_GL_MakeCurrent(NULL, NULL);

    //while (!window->quit) {
    //    SDL_GL_MakeCurrent(window->window, window->context);
//
    //    SDL_Event event;
//
    //    if (SDL_PollEvent(&event)) {
    //        while (event.window.windowID != window->id) {
    //            if (!SDL_WaitEvent(&event)) {
    //                throw("Window", window->name, SDL_GetError());
//
    //                return NULL;
    //            }
    //        }
    //    }
//
    //    if (event.type == SDL_WINDOWEVENT) {
            
    //    }
//
    //    SDL_GL_MakeCurrent(NULL, NULL);
    //}
//
    //for (int i = 0; i < windowPool.count; ++i) {
    //    if (windowPool.windows[i].id == window->id) {
    //        for (int j = i; j < windowPool.count - 1; ++j) {
    //            windowPool.windows[j] = windowPool.windows[j + 1];
    //        }
    //        
    //        windowPool.count -= 1;
    //        
    //        break;
    //    }
    //}
//
    //SDL_DestroyWindow(window->window);
    //SDL_GL_DeleteContext(window->context);
    //SDL_FlushEvent(SDL_WINDOWEVENT);
//
    //free(window);
//
    //return NULL;
}

static const luaL_Reg windowMethods[] = {
    {"selectRender", selectRender},
    {"stopRender", stopRender},
    {"update", update},

    {"isHovering", isHovering},
    {"isFocused", isFocused},
    {"getTitle", getTitle},
    {"getID", getID},
    {NULL, NULL}
};

int createWindow(lua_State *L) {
    int x = luaL_optinteger(L, 1, 300);
    int y = luaL_optinteger(L, 2, 300);
    int width = luaL_optinteger(L, 3, 300);
    int height = luaL_optinteger(L, 4, 200);
    const char *name = luaL_optstring(L, 5, "Vanir window");

    struct sdlWindow *window = malloc(sizeof(struct sdlWindow));

    if (!window) {
        throw("Window", name, "Memory allocation error");

        return 0;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->name = name;
    window->quit = false;
    window->hovering = true;
    window->focused = true;

    struct sdlWindow **udata = (struct sdlWindow **)lua_newuserdata(L, sizeof(struct sdlWindow *));
    *udata = window;

    addMethods(L, "window", windowMethods);
    
    newWindow(window);

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