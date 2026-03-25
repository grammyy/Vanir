#include "../vanir.h"
#include "font.h"

/* ↓ font system stub; implement later ↓ */

// stubs ↓↓↓ stubs ///
int fontCreate(lua_State *L)    { return 0; }
int fontSet(lua_State *L)       { return 0; }
int fontGetSize(lua_State *L)   { lua_pushinteger(L, 0); return 1; }
int fontMeasure(lua_State *L)   { lua_pushinteger(L, 0); return 1; }
int fontDrawText(lua_State *L)  { return 0; }
// stubs ↑↑↑ stubs ///

void destroyAllFonts(void) {}

const luaL_Reg luaFont[] = {
    {"create",   fontCreate},
    {"setFont",  fontSet},
    {"getSize",  fontGetSize},
    {"measure",  fontMeasure},
    {"drawText", fontDrawText},

    {NULL, NULL}
};

int fontInit(lua_State *L) {
    luaL_newlib(L, luaFont);

    return 1;
}
