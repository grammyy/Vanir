#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include "input.h"

int getKey(lua_State *L) {
    int key = luaL_checkinteger(L, 1);
    bool keyHeld = (GetAsyncKeyState(key) & 0x8000) != 0;

    lua_pushboolean(L, keyHeld);

    return 1;
}

const luaL_Reg luaInput[] = {
    {"getKey", getKey},
    {NULL, NULL}
};

int inputInit(lua_State* L) {
    luaL_newlib(L, luaInput);

    return 1;
}