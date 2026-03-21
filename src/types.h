#pragma once

#include "lua_config.h"

void addMethods(lua_State *L, const char *name, const luaL_Reg *methods, const luaL_Reg *meta);

/* ↓ luas color type disassembles to this struct ↓ */
struct color {
    float r, g, b, a;
};

int Vector(lua_State *L);
int Angle(lua_State *L);
int Color(lua_State *L);