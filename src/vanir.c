#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include "hooks.h"
#include "input.h"

typedef struct {
    const char* name;
    lua_CFunction func;
} luaL_reg;

void registerModules(lua_State* L, const luaL_reg* funcs) {
    for (; funcs->name != NULL; ++funcs) {
        lua_pushstring(L, funcs->name);
        funcs->func(L);
        lua_setglobal(L, funcs->name);
    }
}

__declspec(dllexport) int luaopen_vanir(lua_State * L) {
    luaL_dofile(L,"preload.lua");

	const luaL_reg luaReg[] = {
		{"hooks", hooksInit},
        {"input", inputInit},
        {NULL, NULL}
    };

    registerModules(L, luaReg);

	return 1;
}