#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <sys/time.h>
#include "timer.h"

long long timeInMilliseconds(void) {
    struct timeval time;

    gettimeofday(&time, NULL);

    return (((long long)time.tv_sec) * 1000) + (time.tv_usec / 1000);
}

int realtime(lua_State *L) {
    lua_pushinteger(L, timeInMilliseconds());

    return 1;
}

const luaL_Reg luaTimer[] = {
    {"realtime", realtime},
    {NULL, NULL}
};

int timerInit(lua_State* L) {
    luaL_newlib(L, luaTimer);
    
    return 1;
}