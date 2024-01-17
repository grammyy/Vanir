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

#define updateTime 1000 // 1 ms delay (halfs process usage)

static SHORT keyStates[256] = {0};
//pthread_mutex_t keyStatesMutex = PTHREAD_MUTEX_INITIALIZER;

int getKey(lua_State *L) {
    int key = luaL_checkinteger(L, 1);

    lua_pushboolean(L, GetKeyState(key) & keyBitmask);

    return 1;
}

void inputChanged(struct stack *stack, lua_State *L){
    for (int key = 0; key < 256; ++key) {
        SHORT currentState = GetKeyState(key);

        if ((currentState & keyBitmask) != (keyStates[key] & keyBitmask)) {
            stack->callback = createCallback(sizeof(int), integer);
            setCallback(stack->callback, &key);

            stack->status=hook_awaiting;
        }

        keyStates[key] = currentState;
    }
}

void inputPressedHandle(struct stack *stack, lua_State *L){
    for (int key = 0; key < 256; ++key) {
        SHORT currentState = GetKeyState(key);

        if ((currentState & keyBitmask) && ((currentState & keyBitmask) != (keyStates[key] & keyBitmask))) {
            stack->callback = createCallback(sizeof(int), integer);
            setCallback(stack->callback, &key);

            stack->status=hook_awaiting;
        }

        keyStates[key] = currentState;
    }
}

void inputReleasedHandle(struct stack *stack, lua_State *L){
    for (int key = 0; key < 256; ++key) {
        SHORT currentState = GetKeyState(key);

        if (!(currentState & keyBitmask) && ((currentState & keyBitmask) != (keyStates[key] & keyBitmask))) {
            stack->callback = createCallback(sizeof(int), integer);
            setCallback(stack->callback, &key);

            stack->status=hook_awaiting;
        }

        keyStates[key] = currentState;
    }
}

const luaL_Reg luaInput[] = {
    {"getKey", getKey},
    {NULL, NULL}
};

struct hook inputPressed = { "inputPressed", NULL, 0, &inputPressed, inputPressedHandle, hook_idle};
struct hook inputReleased = { "inputReleased", NULL, 0, &inputReleased, inputReleasedHandle, hook_idle};

int inputInit(lua_State* L) {
    pthread_t thread;

    luaL_newlib(L, luaInput);

    registerHook(&pool, inputPressed);
    registerHook(&pool, inputReleased);
    
    //pthread_create(&thread, NULL, inputChanged, (void *)L);
    
    return 1;
}
