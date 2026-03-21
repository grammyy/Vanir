#ifndef VANIR_ENUMS_H
#define VANIR_ENUMS_H

#include "lua_config.h"

typedef struct {
    const char* name;
    int value;
} Enums;

static Enums test[] = {
    {"test", 10},
    
    {NULL, 0}
};

/* ↓ general push command for each enum list ↓ */
void pushEnums(lua_State *L, const Enums *enums) {
    lua_newtable(L);
    
    for (int i = 0; enums[i].name != NULL; ++i) {
        lua_pushstring(L, enums[i].name);
        lua_pushinteger(L, enums[i].value);
        lua_rawset(L, -3);
    }
}

int testEnums(lua_State *L) {
    pushEnums(L, test);

    return 1;
}

#endif