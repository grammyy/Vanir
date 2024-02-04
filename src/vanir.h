#ifndef VANIR
#define VANIR

#include "modules/hooks.h"

typedef struct {
    const char* name;
    lua_CFunction func;
} luaL_reg;

struct color {
    float r, g, b, a;
};

void throw(const char* type, const char* name, const char* error);

#endif