#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "../vanir.h"
#include "hooks.h"
#include "memory.h"

const luaL_Reg luaMemory[] = {
    //{"getKey", getKey},
    {NULL, NULL}
};

//struct hook inputPressed = {"inputPressed", NULL, 0, &inputPressed, inputPressedHandle, hook_idle};

int memoryInit(lua_State* L) {
    luaL_newlib(L, luaMemory);

    //registerHook(&pool, inputPressed);
    
    return 1;
}