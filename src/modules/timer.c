#include <stdint.h>

#include "../vanir.h"
#include "timer.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <time.h>
#endif

static int64_t timeInMilliseconds(void) {
    #ifdef _WIN32
        LARGE_INTEGER freq, counter;

        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&counter);

        return (counter.QuadPart * 1000LL) / freq.QuadPart;
    #else
        struct timespec time;

        clock_gettime(CLOCK_MONOTONIC, &time);

        return ((int64_t)time.tv_sec * 1000) + (time.tv_nsec / 1000000);
    #endif
}

int realtime(lua_State *L) {
    lua_pushnumber(L, (lua_Number)timeInMilliseconds());
    
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