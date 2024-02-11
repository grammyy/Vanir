#ifndef VANIR
#define VANIR

#include "modules/hooks.h"
#include <lua.h>
#include <lauxlib.h>

typedef struct {
    const char* name;
    lua_CFunction func;
} luaL_reg;

struct color {
    float r, g, b, a;
};

void addMethods(lua_State* L, const char* name, const luaL_Reg* methods);
void throw(const char* type, const char* name, const char* error);

#endif