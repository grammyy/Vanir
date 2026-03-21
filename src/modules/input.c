#include <stdbool.h>
#include <stdio.h>

#include "../vanir.h"
#include "hooks.h"
#include "input.h"

#ifdef _WIN32
    #include <windows.h>
    #include <pthread.h>
    #include <unistd.h>

    /* this module currently does nothing for linux, as linux views global keys as a security risk */

    static SHORT keyStates[256] = {0};

    int getKey(lua_State *L) {
        int key = luaL_checkinteger(L, 1);

        lua_pushboolean(L, GetKeyState(key) & keyBitmask);
        
        return 1;
    }

    void inputPressedHandle(struct hook *instance, lua_State *L) {
        for (int key = 0; key < 256; ++key) {
            SHORT currentState = GetKeyState(key);
            
            if ((currentState & keyBitmask) && ((currentState & keyBitmask) != (keyStates[key] & keyBitmask))) {
                instance->callback = createCallback(sizeof(int), integer);
                setCallback(instance->callback, &key);
                
                keyStates[key] = currentState;
                
                instance->status = hook_awaiting;
            }
        }
    }

    void inputReleasedHandle(struct hook *instance, lua_State *L) {
        for (int key = 0; key < 256; ++key) {
            SHORT currentState = GetKeyState(key);
            
            if (!(currentState & keyBitmask) && ((currentState & keyBitmask) != (keyStates[key] & keyBitmask))) {
                instance->callback = createCallback(sizeof(int), integer);
                setCallback(instance->callback, &key);
                
                keyStates[key] = currentState;
                
                instance->status = hook_awaiting;
            }
        }
    }

    static const luaL_Reg luaInput[] = {
        {"getKey", getKey},

        {NULL, NULL}
    };
#else
    static void inputPressedHandle(struct hook *instance, lua_State *L) { (void)instance; (void)L; }
    static void inputReleasedHandle(struct hook *instance, lua_State *L) { (void)instance; (void)L; }

    int getKey(lua_State *L) { (void)L; return 0; }

    static const luaL_Reg luaInput[] = {
        {"getKey", getKey},

        {NULL, NULL}
    };
#endif /* _WIN32 */

struct hook inputPressed  = {"inputPressed",  NULL, 0, &inputPressed,  inputPressedHandle,  hook_idle};
struct hook inputReleased = {"inputReleased", NULL, 0, &inputReleased, inputReleasedHandle, hook_idle};

int inputInit(lua_State *L) {
    luaL_newlib(L, luaInput);

    registerHook(inputPressed);
    registerHook(inputReleased);

    return 1;
}