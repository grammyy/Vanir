#ifndef VANIR_FILES_H
#define VANIR_FILES_H

#include "../lua_config.h"

extern const luaL_Reg fileMethods[];
extern const luaL_Reg fileMeta[];

int filesInit(lua_State *L);

#endif