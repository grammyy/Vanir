#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <pthread.h>
#include "windows.h"
#include "hooks.h"

struct windowPool windowPool = {NULL, 0};

void renderHandle(struct hook *instance, lua_State *L){
    if (windowPool.count>0) {
    }
}

struct hook render = {"render", NULL, 0, &render, renderHandle, hook_update};

void drawSmoothLine(float x1, float y1, float x2, float y2) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f); // Black with full alpha

    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();

    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}

void* newWindow(void* data) {
    struct sdlWindow* window = (struct sdlWindow*)data;
    
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);

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
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        
        SDL_Quit();
        
        return NULL;
    }

    window->context = SDL_GL_CreateContext(window->window);

    if (!window->context) {
        fprintf(stderr, "OpenGL context creation failed: %s\n", SDL_GetError());
        
        SDL_DestroyWindow(window->window);
        SDL_Quit();
        
        return NULL;
    }

    window->quit=0;

    struct sdlWindow* temp = (struct sdlWindow*)realloc(windowPool.windows, (windowPool.count + 1) * sizeof(struct sdlWindow));

    if (temp != NULL) {
        windowPool.windows = temp;
        windowPool.windows[windowPool.count] = *window;
        windowPool.count += 1;
    } else {
        //throw("Hook", "pool", "Memory allocation error");
    }

    while (!window->quit) {
        SDL_GL_MakeCurrent(window->window, window->context);

        SDL_Event event;
        
        if (SDL_PollEvent(&event)) {
            while (event.window.windowID != window->id) {
                if (!SDL_WaitEvent(&event)) {
                    fprintf(stderr, "SDL_WaitEvent error: %s\n", SDL_GetError());
                    
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

                    SDL_GL_SwapWindow(window->window);

                    break;
                case SDL_WINDOWEVENT_CLOSE:
                    window->quit=1;

                    break;
            }
        }

        SDL_GL_MakeCurrent(NULL, NULL);
    }

    SDL_DestroyWindow(window->window);
    SDL_GL_DeleteContext(window->context);

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

        return 1;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->name = name;

    pthread_t glThread;

    if (pthread_create(&glThread, NULL, newWindow, (void *)window) != 0) {
        fprintf(stderr, "Failed to create OpenGL window thread\n");
        
        free(window);  // Free allocated memory
    }

    return 1;
}

int drawLine(lua_State *L) {
    if (windowPool.count > 0) {
        if (lua_gettop(L) == 4) {
            SDL_GL_MakeCurrent(windowPool.windows[0].window, windowPool.windows[0].context);

            float x1 = lua_tonumber(L, 1);
            float y1 = lua_tonumber(L, 2);
            float x2 = lua_tonumber(L, 3);
            float y2 = lua_tonumber(L, 4);

            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            drawSmoothLine(x1, y1, x2, y2);

            SDL_GL_SwapWindow(windowPool.windows[0].window);

            SDL_GL_MakeCurrent(NULL, NULL);
        } else {
            //fprintf(stderr, "drawLine requires exactly 4 number arguments\n");
        }
    }

    return 0;
}

const luaL_Reg luaWindows[] = {
    {"createWindow", createWindow},
    {"drawLine", drawLine},
    {NULL, NULL}
};

int windowsInit(lua_State* L) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());

        return 1;
    }
    
    luaL_newlib(L, luaWindows);

    registerHook(&pool, render);

    return 1;
}