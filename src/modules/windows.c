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

struct hook onHoverChange = { "onHoverChange", NULL, 0, &onHoverChange, NULL, hook_idle};
struct hook onFocusChange = { "onFocusChange", NULL, 0, &onFocusChange, NULL, hook_idle};
struct hook onResize = { "onResize", NULL, 0, &onResize, NULL, hook_idle};
struct hook onEvent = { "onEvent", NULL, 0, &onEvent, NULL, hook_idle};
struct hook onClose = { "onClose", NULL, 0, &onClose, NULL, hook_idle};
struct hook onOpen = { "onOpen", NULL, 0, &onOpen, NULL, hook_idle};

void glAspectRatio(int width, int height) {
    float aspectRatio = (float) width / (float) height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (aspectRatio >= 1.0) {
        glOrtho(0, width, height, 0, -1.0, 1.0);
    } else {
        float halfHeight = (float) width / (2 * aspectRatio);
        float halfWidth = halfHeight * aspectRatio;
        
        glOrtho(0, width, height, 0, -1.0, 1.0);
    }

    glMatrixMode(GL_MODELVIEW);
}

void renderHandle(struct hook *instance, lua_State *L) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        onEvent.status=hook_awaiting;
                    
        setCallback(onEvent.callback, &(int){event.type});

        if (event.type == SDL_WINDOWEVENT) {
            for (size_t i = 0; i < windowPool.count; ++i) {
                if (windowPool.windows[i]->id == event.window.windowID) {
                    SDL_GL_MakeCurrent(windowPool.windows[i]->window, windowPool.windows[i]->context);
                    
                    switch(event.window.event){
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            int width = event.window.data1;
                            int height = event.window.data2;
                            
                            windowPool.windows[i]->width = width;
                            windowPool.windows[i]->height = height;
                            onResize.status=hook_awaiting;
                            
                            glViewport(0, 0, width, height);
                            glAspectRatio(width, height);

                            break;
                        case SDL_WINDOWEVENT_CLOSE:
                            windowPool.windows[i]->quit = true;
                            onClose.status=hook_awaiting;

                            SDL_GL_DeleteContext(windowPool.windows[i]->context);
                            SDL_DestroyWindow(windowPool.windows[i]->window);
                            
                            lua_pushnil(L);
                            lua_rawseti(L, LUA_REGISTRYINDEX, windowPool.windows[i]->ref);

                            for (int j = i; j < windowPool.count - 1; ++j) {
                                windowPool.windows[j] = windowPool.windows[j + 1];
                            }

                            windowPool.windows[windowPool.count] = NULL;
                            windowPool.count -= 1;

                            break;
                        case SDL_WINDOWEVENT_ENTER:
                            windowPool.windows[i]->hovering = true;
                            onHoverChange.status=hook_awaiting;
                            
                            setCallback(onHoverChange.callback, &(bool){true});

                            break;
                        case SDL_WINDOWEVENT_LEAVE:
                            windowPool.windows[i]->hovering = false;
                            onHoverChange.status=hook_awaiting;
                            
                            setCallback(onHoverChange.callback, &(bool){false});

                            break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            windowPool.windows[i]->focused = true;
                            onFocusChange.status=hook_awaiting;

                            setCallback(onFocusChange.callback, &(bool){true});

                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            windowPool.windows[i]->focused = false;
                            onFocusChange.status=hook_awaiting;

                            setCallback(onFocusChange.callback, &(bool){false});

                            break;
                    }

                    SDL_GL_MakeCurrent(NULL, NULL);

                    return;
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

int getMouse(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");

    if (SDL_GetMouseFocus() != (*window)->window) {
        lua_pushnil(L);
        lua_pushnil(L);
    
        return 2;
    }
    
    int x, y;

    SDL_GetMouseState(&x, &y);

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);

    return 2;
}

int getSize(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");

    lua_pushinteger(L, (*window)->width);
    lua_pushinteger(L, (*window)->height);

    return 2;
}

int getMonitorIndex(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");

    int index = SDL_GetWindowDisplayIndex((*window)->window);

    lua_pushinteger(L, index);
    
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
    //SDL_WINDOW_BORDERLESS

    window->id=SDL_GetWindowID(window->window);

    if (!window->window) {
        throw("Window", window->name, SDL_GetError());

        return;
    }

    window->context = SDL_GL_CreateContext(window->window);

    if (!window->context) {
        throw("Window", window->name, "Failed to create OpenGL context");

        SDL_DestroyWindow(window->window);
        
        return;
    }

    struct sdlWindow **temp = (struct sdlWindow **)realloc(windowPool.windows, (windowPool.count + 1) * sizeof(struct sdlWindow *));

    if (temp != NULL) {
        windowPool.windows = temp;
        windowPool.windows[windowPool.count] = window;
        windowPool.count += 1;
    } else {
        throw("Window", window->name, "Memory allocation error");

        SDL_GL_DeleteContext(window->context);
        SDL_DestroyWindow(window->window);

        return;
    }
    
    SDL_GL_MakeCurrent(window->window, window->context);

    glAspectRatio(window->width, window->height);

    // Enable anti-aliasing for lines
    glEnable(GL_BLEND);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable anti-aliasing for polygons -> also causes weird lines for filled polygons - fix later
    //glEnable(GL_POLYGON_SMOOTH);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    // Enable anti-aliasing for textures
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);

    // Enable anti-aliasing for fragment shader derivatives
    glEnable(GL_FRAGMENT_SHADER_DERIVATIVE_HINT);
    glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);

    //glEnable(GL_TEXTURE_2D);

    SDL_GL_MakeCurrent(NULL, NULL);
}

static const luaL_Reg windowMethods[] = {
    {"selectRender", selectRender},
    {"stopRender", stopRender},
    {"update", update},

    {"isHovering", isHovering},
    {"isFocused", isFocused},
    {"getTitle", getTitle},
    {"getID", getID},
    {"getMouse", getMouse},
    {"getSize", getSize},
    {"getMonitorIndex", getMonitorIndex},
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
    window->ref = luaL_ref(L, LUA_REGISTRYINDEX); //doesnt seem to push nil to tables, fix later

    struct sdlWindow **udata = (struct sdlWindow **)lua_newuserdata(L, sizeof(struct sdlWindow *));
    *udata = window;

    addMethods(L, "window", windowMethods);
    newWindow(window);

    onHoverChange.callback = createCallback(sizeof(bool), lua_bool);
    onFocusChange.callback = createCallback(sizeof(bool), lua_bool);
    onEvent.callback = createCallback(sizeof(int), integer);
    onOpen.status=hook_awaiting;
    
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
    registerHook(&pool, onHoverChange);
    registerHook(&pool, onFocusChange);
    registerHook(&pool, onResize);
    registerHook(&pool, onEvent);
    registerHook(&pool, onClose);
    registerHook(&pool, onOpen);

    return 1;
}