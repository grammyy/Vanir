#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <Windows.h>
#include <stdio.h>
#include "modules/hooks.h"
#include "modules/input.h"
#include "modules/windows.h"
#include "modules/render.h"
#include "modules/timer.h"
#include "enums.h"
#include "vanir.h"

int Color(lua_State* L) {
    //might need to automantically handle values to their gl counterpart for performance
    int r = luaL_optnumber(L, 1, 255.0f);
    int g = luaL_optnumber(L, 2, 255.0f);
    int b = luaL_optnumber(L, 3, 255.0f);
    int a = luaL_optnumber(L, 4, 255.0f);

    struct color color = {r, g, b, a};

    lua_newtable(L);

    lua_pushstring(L, "r");
    lua_pushnumber(L, color.r);
    lua_settable(L, -3);

    lua_pushstring(L, "g");
    lua_pushnumber(L, color.g);
    lua_settable(L, -3);

    lua_pushstring(L, "b");
    lua_pushnumber(L, color.b);
    lua_settable(L, -3);

    lua_pushstring(L, "a");
    lua_pushnumber(L, color.a);
    lua_settable(L, -3);

    return 1;
}

void registerGlobals(lua_State* L, const luaL_reg* funcs) {
    for (; funcs->name != NULL; ++funcs) {
        lua_pushstring(L, funcs->name);

        funcs->func(L);

        lua_setglobal(L, funcs->name);
    }
}

void throw(const char* type, const char* name, const char* error) {
    printf("%s \"%s\" errored with: %s\n", type, name, error);
}

__declspec(dllexport) int luaopen_vanir(lua_State * L) {
    luaL_dofile(L, "preload.lua");

    lua_newtable(L);

    lua_pushstring(L, "r");
    lua_pushnumber(L, 255.0f);
    lua_settable(L, -3);

    lua_pushstring(L, "g");
    lua_pushnumber(L, 255.0f);
    lua_settable(L, -3);

    lua_pushstring(L, "b");
    lua_pushnumber(L, 255.0f);
    lua_settable(L, -3);

    lua_pushstring(L, "a");
    lua_pushnumber(L, 255.0f);
    lua_settable(L, -3);

    lua_setglobal(L, "_rendercolor");

    lua_pushcfunction(L, Color);
    lua_setglobal(L, "Color");

    const luaL_reg luaReg[] = {
        // ↓ modules ↓ ///
        {"hooks", hooksInit},
        {"input", inputInit},
        {"windows", windowsInit},
        {"render", renderInit},
        {"timer", timerInit},

        // ↓ enums ↓ ///
        {"gl", renderEnums},
        {NULL, NULL}
    };

    registerGlobals(L, luaReg);

    return 0;
}
