#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hooks.h"
#include "input.h"

static int l_messagebox(lua_State * L) {
	const char * s = luaL_checkstring(L, 1);

	MessageBox(NULL,s,"messagebox",MB_OK);

	return 0;
}

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
	//pid_t pid = getpid();
    
    luaL_dofile(L,"preload.lua");

	lua_pushcfunction(L, l_messagebox);
	lua_setglobal(L,"messagebox");

	const luaL_reg luaReg[] = {
		{"hooks", hooksInit},
        {"input", inputInit},
        {NULL, NULL}
    };

    // Register the "vanir" table with functions
    registerModules(L, luaReg);
    //runHook(&think, L);
	return 1;
}