#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <sys/time.h>
#include "timer.h"

long long timeInMilliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

int realtime(lua_State *L) {
    long long result = timeInMilliseconds();
    lua_pushinteger(L, result);
    return 1;  // Number of return values
}

const luaL_Reg luaTimer[] = {
    {"realtime", realtime},
    {NULL, NULL}
};

int timerInit(lua_State* L) {
    luaL_newlib(L, luaTimer);
    
    return 1;
}