#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../vanir.h"
#include "timer.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <time.h>
#endif

/* ↓ per-timer state ↓ */
struct Timer {
    char    *name;
    double   delay;       /* ms between fires */
    int      reps;        /* -1 = infinite */
    int      repsLeft;
    int      funcRef;     /* lua registry ref for the callback */
    double   nextFire;    /* absolute ms timestamp for next fire */
    bool     paused;
    double   pausedAt;    /* realtime() when paused; negative means not paused */
    bool     active;
};

static struct Timer *timerPool  = NULL;
static int           timerCount = 0;
static int           timerCap   = 0;

/* ↓ simple timers are fire-once with no name; stored in the same pool ↓ */
static const char *SIMPLE_PREFIX = "__simple_";
static int simpleCounter = 0;

/* ↓ per-frame timing ↓ */
static double lastFrameTime = 0.0;
static double frameDelta    = 0.0; /* ms */

/* ↓ server-style curtime; seconds since vanir started ↓ */
static double startTime = 0.0;

static int64_t rawMilliseconds(void) {
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

static double nowMs(void) {
    return (double)rawMilliseconds();
}

/* ↓ systime — unix epoch in seconds, float ↓ */
static double nowSec(void) {
#ifdef _WIN32
    FILETIME ft;
    ULARGE_INTEGER uli;

    GetSystemTimeAsFileTime(&ft);
    uli.LowPart  = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    /* ↓ windows epoch is 1601-01-01; unix is 1970-01-01; delta = 116444736000000000 100ns ticks ↓ */
    return (double)(uli.QuadPart - 116444736000000000ULL) / 10000000.0;
#else
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#endif
}

/* ↓ find a timer by name; returns NULL if not found ↓ */
static struct Timer *findTimer(const char *name) {
    for (int i = 0; i < timerCount; ++i) {
        if (timerPool[i].active && strcmp(timerPool[i].name, name) == 0)
            return &timerPool[i];
    }

    return NULL;
}

/* ↓ find a free slot or grow the pool ↓ */
static struct Timer *allocTimer(void) {
    /* ↓ reuse a dead slot first ↓ */
    for (int i = 0; i < timerCount; ++i) {
        if (!timerPool[i].active)
            return &timerPool[i];
    }

    /* ↓ grow ↓ */
    if (timerCount >= timerCap) {
        int newCap = timerCap == 0 ? 8 : timerCap * 2;
        struct Timer *temp = realloc(timerPool, newCap * sizeof(struct Timer));

        if (!temp) {
            throw("Timer", "pool", "Memory allocation error");

            return NULL;
        }

        timerPool = temp;
        timerCap  = newCap;
    }

    return &timerPool[timerCount++];
}

/* ↓ tick all active timers; called from the think hook or manually ↓ */
void tickTimers(lua_State *L) {
    double now = nowMs();

    /* ↓ update frametime ↓ */
    if (lastFrameTime > 0.0)
        frameDelta = now - lastFrameTime;

    lastFrameTime = now;

    for (int i = 0; i < timerCount; ++i) {
        struct Timer *t = &timerPool[i];

        if (!t->active || t->paused)
            continue;

        if (now < t->nextFire)
            continue;

        /* ↓ advance by delay rather than snapping to now; avoids drift ↓ */
        t->nextFire += t->delay;

        /* ↓ if we fell very far behind (e.g. paused), skip to current ↓ */
        if (t->nextFire < now)
            t->nextFire = now + t->delay;

        /* ↓ call the lua function ↓ */
        if (t->funcRef != LUA_NOREF) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, t->funcRef);

            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                throw("Timer", t->name, lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        }

        /* ↓ count down reps ↓ */
        if (t->reps >= 0) {
            t->repsLeft -= 1;

            if (t->repsLeft <= 0) {
                luaL_unref(L, LUA_REGISTRYINDEX, t->funcRef);
                free(t->name);
                memset(t, 0, sizeof(struct Timer));
            }
        }
    }
}

/* ↓ realtime() → number — monotonic ms since process start ↓ */
int realtime(lua_State *L) {
    lua_pushnumber(L, nowMs());

    return 1;
}

/* ↓ systime() → number — unix epoch seconds as float ↓ */
int systime(lua_State *L) {
    lua_pushnumber(L, nowSec());

    return 1;
}

/* ↓ curtime() → number — seconds since vanirInit ↓ */
int curtime(lua_State *L) {
    if (startTime == 0.0)
        startTime = nowMs();

    lua_pushnumber(L, (nowMs() - startTime) / 1000.0);

    return 1;
}

/* ↓ frametime() → number — ms of the last frame ↓ */
int frametime(lua_State *L) {
    lua_pushnumber(L, frameDelta);

    return 1;
}

/* ↓ timer.create(name, delay, reps, func) → None ↓ */
int timerCreate(lua_State *L) {
    const char *name  = luaL_checkstring(L, 1);
    double      delay = luaL_checknumber(L, 2);
    int         reps  = (int)luaL_checkinteger(L, 3); /* -1 for infinite */

    luaL_checktype(L, 4, LUA_TFUNCTION);

    /* ↓ replace existing timer with same name ↓ */
    struct Timer *t = findTimer(name);

    if (t) {
        luaL_unref(L, LUA_REGISTRYINDEX, t->funcRef);
        free(t->name);
    } else {
        t = allocTimer();

        if (!t) return 0;
    }

    lua_pushvalue(L, 4);

    t->name      = strdup(name);
    t->delay     = delay;
    t->reps      = reps;
    t->repsLeft  = reps;
    t->funcRef   = luaL_ref(L, LUA_REGISTRYINDEX);
    t->nextFire  = nowMs() + delay;
    t->paused    = false;
    t->pausedAt  = 0.0;
    t->active    = true;

    return 0;
}

/* ↓ timer.simple(delay, func) → None — one-shot anonymous timer ↓ */
int timerSimple(lua_State *L) {
    double delay = luaL_checknumber(L, 1);

    luaL_checktype(L, 2, LUA_TFUNCTION);

    char name[64];
    snprintf(name, sizeof(name), "%s%d", SIMPLE_PREFIX, simpleCounter++);

    lua_pushstring(L, name);
    lua_insert(L, 1);               /* name, delay, func */
    lua_pushinteger(L, 1);          /* name, delay, func, 1 */
    lua_insert(L, 3);               /* name, delay, 1, func */

    return timerCreate(L);
}

/* ↓ timer.remove(name) → None ↓ */
int timerRemove(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    struct Timer *t  = findTimer(name);

    if (!t) return 0;

    luaL_unref(L, LUA_REGISTRYINDEX, t->funcRef);
    free(t->name);
    memset(t, 0, sizeof(struct Timer));

    return 0;
}

/* ↓ timer.exists(name) → boolean ↓ */
int timerExists(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);

    lua_pushboolean(L, findTimer(name) != NULL);

    return 1;
}

/* ↓ timer.start(name) → boolean — restart a stopped/finished timer from now ↓ */
int timerStart(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    struct Timer *t  = findTimer(name);

    if (!t) { lua_pushboolean(L, 0); return 1; }

    t->nextFire = nowMs() + t->delay;
    t->paused   = false;
    t->repsLeft = t->reps;

    lua_pushboolean(L, 1);

    return 1;
}

/* ↓ timer.stop(name) → boolean — stops (deactivates) a timer without removing it ↓ */
int timerStop(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    struct Timer *t  = findTimer(name);

    if (!t) { lua_pushboolean(L, 0); return 1; }

    t->active = false;

    lua_pushboolean(L, 1);

    return 1;
}

/* ↓ timer.pause(name) → boolean ↓ */
int timerPause(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    struct Timer *t  = findTimer(name);

    if (!t || t->paused) { lua_pushboolean(L, 0); return 1; }

    t->paused   = true;
    t->pausedAt = nowMs();

    lua_pushboolean(L, 1);

    return 1;
}

/* ↓ timer.unpause(name) → boolean ↓ */
int timerUnpause(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    struct Timer *t  = findTimer(name);

    if (!t || !t->paused) { lua_pushboolean(L, 0); return 1; }

    /* ↓ push nextFire forward by the time spent paused ↓ */
    double pausedFor = nowMs() - t->pausedAt;
    t->nextFire += pausedFor;
    t->paused    = false;
    t->pausedAt  = 0.0;

    lua_pushboolean(L, 1);

    return 1;
}

/* ↓ timer.toggle(name) → boolean — current paused state after toggle ↓ */
int timerToggle(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    struct Timer *t  = findTimer(name);

    if (!t) { lua_pushboolean(L, 0); return 1; }

    if (t->paused) {
        lua_settop(L, 1);
        timerUnpause(L);
        lua_pushboolean(L, 0); /* now unpaused */
    } else {
        lua_settop(L, 1);
        timerPause(L);
        lua_pushboolean(L, 1); /* now paused */
    }

    return 1;
}

/* ↓ timer.adjust(name, delay, reps|nil, func|nil) → boolean ↓ */
int timerAdjust(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    double      delay = luaL_checknumber(L, 2);
    struct Timer *t   = findTimer(name);

    if (!t) { lua_pushboolean(L, 0); return 1; }

    t->delay = delay;

    if (!lua_isnil(L, 3))
        t->reps = t->repsLeft = (int)luaL_checkinteger(L, 3);

    if (!lua_isnil(L, 4) && lua_isfunction(L, 4)) {
        luaL_unref(L, LUA_REGISTRYINDEX, t->funcRef);
        lua_pushvalue(L, 4);
        t->funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    lua_pushboolean(L, 1);

    return 1;
}

/* ↓ timer.timeleft(name) → number (negative if paused) | nil ↓ */
int timerTimeleft(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    struct Timer *t  = findTimer(name);

    if (!t) { lua_pushnil(L); return 1; }

    double left;

    if (t->paused) {
        /* ↓ return negative to indicate paused; magnitude is remaining ms ↓ */
        left = -(t->nextFire - t->pausedAt);
    } else {
        left = t->nextFire - nowMs();
    }

    lua_pushnumber(L, left);

    return 1;
}

/* ↓ timer.repsleft(name) → number | nil ↓ */
int timerRepsleft(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    struct Timer *t  = findTimer(name);

    if (!t) { lua_pushnil(L); return 1; }

    lua_pushinteger(L, t->repsLeft);

    return 1;
}

/* ↓ timer.getTimersLeft() → number — count of active, unfinished timers ↓ */
int timerGetTimersLeft(lua_State *L) {
    int count = 0;

    for (int i = 0; i < timerCount; ++i) {
        if (timerPool[i].active)
            count += 1;
    }

    lua_pushinteger(L, count);

    return 1;
}

const luaL_Reg luaTimer[] = {
    {"realtime",      realtime},
    {"systime",       systime},
    {"curtime",       curtime},
    {"frametime",     frametime},
    {"create",        timerCreate},
    {"simple",        timerSimple},
    {"remove",        timerRemove},
    {"exists",        timerExists},
    {"start",         timerStart},
    {"stop",          timerStop},
    {"pause",         timerPause},
    {"unpause",       timerUnpause},
    {"toggle",        timerToggle},
    {"adjust",        timerAdjust},
    {"timeleft",      timerTimeleft},
    {"repsleft",      timerRepsleft},
    {"getTimersLeft", timerGetTimersLeft},

    {NULL, NULL}
};

int timerInit(lua_State* L) {
    startTime = nowMs();

    luaL_newlib(L, luaTimer);

    return 1;
}
