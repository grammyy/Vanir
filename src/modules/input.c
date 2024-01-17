#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "vanir.h"
#include "hooks.h"
#include "input.h"

static SHORT keyStates[256] = {0};

int getKey(lua_State *L) {
    int key = luaL_checkinteger(L, 1);

    lua_pushboolean(L, GetKeyState(key) & keyBitmask);

    return 1;
}

void inputPressedHandle(struct stack *stack, lua_State *L){
    for (int key = 0; key < 256; ++key) {
        SHORT currentState = GetKeyState(key);

        if ((currentState & keyBitmask) && ((currentState & keyBitmask) != (keyStates[key] & keyBitmask))) {
            stack->callback = createCallback(sizeof(int), integer);
            setCallback(stack->callback, &key);

            stack->status=hook_awaiting;
            
            keyStates[key] = currentState;
        }
    }
}

void inputReleasedHandle(struct stack *stack, lua_State *L){
    for (int key = 0; key < 256; ++key) {
        SHORT currentState = GetKeyState(key);

        if (!(currentState & keyBitmask) && ((currentState & keyBitmask) != (keyStates[key] & keyBitmask))) {
            stack->callback = createCallback(sizeof(int), integer);
            setCallback(stack->callback, &key);

            stack->status=hook_awaiting;
            
            keyStates[key] = currentState;
        }
    }
}

const luaL_Reg luaInput[] = {
    {"getKey", getKey},
    {NULL, NULL}
};

struct hook inputPressed = { "inputPressed", NULL, 0, &inputPressed, inputPressedHandle, hook_idle};
struct hook inputReleased = { "inputReleased", NULL, 0, &inputReleased, inputReleasedHandle, hook_idle};

int inputInit(lua_State* L) {
    luaL_newlib(L, luaInput);

    registerHook(&pool, inputPressed);
    registerHook(&pool, inputReleased);
    
    return 1;
}
