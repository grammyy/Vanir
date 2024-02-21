#include <SDL.h>
#include "../vanir.h"
#include "system.h"

void queryMonitor(lua_State *L, int index) {
    SDL_Rect bounds;
    
    SDL_GetDisplayBounds(index, &bounds);

    lua_newtable(L);

    lua_pushinteger(L, index);
    lua_setfield(L, -2, "index");
    lua_pushinteger(L, bounds.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, bounds.y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, bounds.w);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, bounds.h);
    lua_setfield(L, -2, "height");

    SDL_DisplayMode display;
    
    SDL_GetDesktopDisplayMode(index, &display);

    lua_pushinteger(L, display.refresh_rate);
    lua_setfield(L, -2, "refreshRate");
    lua_pushstring(L, SDL_GetDisplayName(index)); //only returns type name, is known sdl bug
    lua_setfield(L, -2, "name");

    SDL_DisplayOrientation mode = SDL_GetDisplayOrientation(index);

    switch(mode) {
        case SDL_ORIENTATION_PORTRAIT_FLIPPED:
            lua_pushinteger(L, 4); break;
        case SDL_ORIENTATION_PORTRAIT:
            lua_pushinteger(L, 3); break;
        case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
            lua_pushinteger(L, 2); break;
        case SDL_ORIENTATION_LANDSCAPE:
            lua_pushinteger(L, 1); break;
        default:
            lua_pushinteger(L, 0); break;
    }

    lua_setfield(L, -2, "orientation");

    float ddpi, hdpi, vdpi;

    SDL_GetDisplayDPI(index, &ddpi, &hdpi, &vdpi); //not really sure what this is for, but seems useful

    lua_pushnumber(L, ddpi);
    lua_setfield(L, -2, "ddpi");
    lua_pushnumber(L, hdpi);
    lua_setfield(L, -2, "hdpi");
    lua_pushnumber(L, vdpi);
    lua_setfield(L, -2, "vdpi");
}

int getMonitorCount(lua_State *L) {
    int displays = SDL_GetNumVideoDisplays();

    lua_pushinteger(L, displays);

    return 1;
}

int getMonitor(lua_State *L) {
    int index = luaL_checkinteger(L, 1);
    
    queryMonitor(L, index);

    return 1;
}

int getMainMonitor(lua_State *L) {
    lua_pushinteger(L, 0);

    getMonitor(L);

    return 1;
}

const luaL_Reg luaSystem[] = {
    {"getMonitorCount", getMonitorCount},
    {"getMainMonitor", getMainMonitor},
    {"getMonitor", getMonitor},
    {NULL, NULL}
};

int systemInit(lua_State* L) {
    luaL_newlib(L, luaSystem);
    
    return 1;
}