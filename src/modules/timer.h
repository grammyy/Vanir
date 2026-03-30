#ifndef TIMER
#define TIMER

#include <stdbool.h>

int timerInit(lua_State* L);

/* ↓ tick all active timers; hook this into the think loop ↓ */
void tickTimers(lua_State *L);

#endif