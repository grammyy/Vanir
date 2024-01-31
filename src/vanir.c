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

void throw(const char* type, const char* name, const char* error) {
    printf("%s \"%s\" errored with: %s.\n", type, name, error);
}

__declspec(dllexport) int luaopen_vanir(lua_State * L) {
    luaL_dofile(L, "preload.lua");

    const luaL_reg luaReg[] = {
        {"hooks", hooksInit},
        {"input", inputInit},
        {"windows", windowsInit},
        {"render", renderInit},
        {"timer", timerInit},
        {NULL, NULL}
    };

    registerModules(L, luaReg);

    return 1;
}
