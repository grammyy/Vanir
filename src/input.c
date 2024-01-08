#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "hooks.h"
#include "input.h"

#define keyBitmask 0x80
#define updateTime 1000 // 1 ms delay (halfs process usage)

static SHORT keyStates[256] = {0};
pthread_mutex_t keyStatesMutex = PTHREAD_MUTEX_INITIALIZER;

int getKey(lua_State *L) {
    int key = luaL_checkinteger(L, 1);
    bool keyHeld = (GetAsyncKeyState(key) & 0x8000) != 0;

    lua_pushboolean(L, keyHeld);

    return 1;
}

// Thread to monitor input changes
void* inputChanged(void *arg) {
    lua_State *L = (lua_State *)arg;

    while (1) {
        for (int key = 0; key < 256; ++key) {
            SHORT currentState = GetKeyState(key);

            pthread_mutex_lock(&keyStatesMutex);

            if ((currentState & keyBitmask) != (keyStates[key] & keyBitmask)) {
                struct callbacks* intCallback = createCallback(sizeof(int), number);
                setCallback(intCallback, &key);

                if ((currentState & keyBitmask) != 0) {
                    runHook(&inputPressed, L, intCallback);
                } else {
                    runHook(&inputReleased, L, intCallback);
                }
            }

            keyStates[key] = currentState;

            pthread_mutex_unlock(&keyStatesMutex);
        }

        usleep(updateTime);
    }

    return NULL;
}

const luaL_Reg luaInput[] = {
    {"getKey", getKey},
    {NULL, NULL}
};

int inputInit(lua_State* L) {
    pthread_t thread;

    luaL_newlib(L, luaInput);

    pool.hooks[1] = inputPressed;
    pool.hooks[2] = inputReleased;

    pthread_create(&thread, NULL, inputChanged, (void *)L);
    
    return 1;
}
