#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "windows.h"
#include "../graphics/render.h"
#include "render.h"

int line(lua_State *L) {
    SDL_GL_MakeCurrent(windowPool.windows[0].window, windowPool.windows[0].context);

    float x1 = lua_tonumber(L, 1);
    float y1 = lua_tonumber(L, 2);
    float x2 = lua_tonumber(L, 3);
    float y2 = lua_tonumber(L, 4);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawLine(x1, y1, x2, y2);

    SDL_GL_SwapWindow(windowPool.windows[0].window);

    SDL_GL_MakeCurrent(NULL, NULL);

    return 0;
}

const luaL_Reg luaRender[] = {
    {"drawLine",line},
    {NULL, NULL}
};

int renderInit(lua_State* L) {
    luaL_newlib(L, luaRender);

    return 1;
}