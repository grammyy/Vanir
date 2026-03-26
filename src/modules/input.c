#include <stdbool.h>
#include <stdio.h>

#include "../vanir.h"
#include "../enums.h"
#include "hooks.h"
#include "input.h"

/* ↓ reverse-lookup: code -> name string, checks keys then mouse buttons, nil if not found ↓ */
static int getInputName(lua_State *L) {
    int code = (int)luaL_checkinteger(L, 1);


    for (int i = 0; keys[i].name != NULL; ++i) {
        if (keys[i].value == code) {
            lua_pushstring(L, keys[i].name);
            return 1;
        }
    }

    for (int i = 0; mouseButtons[i].name != NULL; ++i) {
        if (mouseButtons[i].value == code) {
            lua_pushstring(L, mouseButtons[i].name);

            throw("test", "test", mouseButtons[i].name);

            return 1;
        }
    }

    lua_pushnil(L);

    return 1;
}

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

    void keyPressHandle(struct hook *instance, lua_State *L) {
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

    void keyReleaseHandle(struct hook *instance, lua_State *L) {
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
        {"getKey",     getKey},
        {"getKeyName", getInputName},

        {NULL, NULL}
    };
#else
    static void keyPressHandle(struct hook *instance, lua_State *L) { (void)instance; (void)L; }
    static void keyReleaseHandle(struct hook *instance, lua_State *L) { (void)instance; (void)L; }

    int getKey(lua_State *L) { (void)L; return 0; }

    static const luaL_Reg luaInput[] = {
        {"getKey",     getKey},
        {"getKeyName", getInputName},

        {NULL, NULL}
    };
#endif /* _WIN32 */

struct hook keyPress    = {"keyPress",    NULL, 0, &keyPress,    keyPressHandle,   hook_idle};
struct hook keyRelease  = {"keyRelease",  NULL, 0, &keyRelease,  keyReleaseHandle, hook_idle};

/* ↓ inputPressed / inputReleased fire from the GLFW key callback — works on both platforms ↓ */
struct hook inputPressed  = {"inputPressed",  NULL, 0, &inputPressed,  NULL, hook_idle};
struct hook inputReleased = {"inputReleased", NULL, 0, &inputReleased, NULL, hook_idle};

/* ↓ glfw key callback; registered per-window in windows.c via glfwSetKeyCallback ↓ */
void cbKey(GLFWwindow *win, int key, int scancode, int action, int mods) {
    (void)win; (void)scancode; (void)mods;

    if (action == GLFW_PRESS) {
        inputPressed.callback = createCallback(sizeof(int), integer);
        setCallback(inputPressed.callback, &key);

        inputPressed.status = hook_awaiting;
    } else if (action == GLFW_RELEASE) {
        inputReleased.callback = createCallback(sizeof(int), integer);
        setCallback(inputReleased.callback, &key);

        inputReleased.status = hook_awaiting;
    }
}

/* ↓ glfw mouse button callback; registered per-window via glfwSetMouseButtonCallback ↓ */
/* ↓ button codes match GLFW_MOUSE_BUTTON_* and the MOUSE enum exposed to Lua       ↓ */
void cbMouseButton(GLFWwindow *win, int button, int action, int mods) {
    (void)win; (void)mods;

    if (action == GLFW_PRESS) {
        inputPressed.callback = createCallback(sizeof(int), integer);
        setCallback(inputPressed.callback, &button);

        inputPressed.status = hook_awaiting;
    } else if (action == GLFW_RELEASE) {
        inputReleased.callback = createCallback(sizeof(int), integer);
        setCallback(inputReleased.callback, &button);

        inputReleased.status = hook_awaiting;
    }
}

int inputInit(lua_State *L) {
    luaL_newlib(L, luaInput);

    registerHook(keyPress);
    registerHook(keyRelease);
    registerHook(inputPressed);
    registerHook(inputReleased);

    return 1;
}
