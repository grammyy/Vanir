#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "windows.h"
#include "../graphics/render.h"
#include "render.h"

// window methods ↓↓↓ window methods ///
int selectRender(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");
    
    SDL_GL_MakeCurrent((*window)->window, (*window)->context);
}

int stopRender(lua_State *L) {
    SDL_GL_MakeCurrent(NULL, NULL);
}

int update(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");
    
    SDL_GL_SwapWindow((*window)->window);
}

int setQuality(lua_State *L) {
    GLenum target = (GLenum)lua_tonumber(L, 1);
    GLenum quality = (GLenum)lua_tonumber(L, 2);

    glHint(target, quality);
}
// window methods ↑↑↑ window methods ///

int line(lua_State *L) {
    float x1 = lua_tonumber(L, 1);
    float y1 = lua_tonumber(L, 2);
    float x2 = lua_tonumber(L, 3);
    float y2 = lua_tonumber(L, 4);

    drawLine(x1, y1, x2, y2);

    return 0;
}

int clear() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    return 0;
}

const luaL_Reg luaRender[] = {
    {"drawLine", line},
    {"clear", clear},
    {NULL, NULL}
};

int renderInit(lua_State* L) {
    luaL_newlib(L, luaRender);

    return 1;
}