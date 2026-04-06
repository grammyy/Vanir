#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "../vanir.h"
#include "../enums.h"
#include "hooks.h"
#include "input.h"
#include "windows.h"

extern struct windowPool windowPool;

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

            return 1;
        }
    }

    lua_pushnil(L);

    return 1;
}

/* ↓ input.lookupBinding(name) → code | nil; searches keys then mouse buttons by name ↓ */
static int lookupBinding(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);

    for (int i = 0; keys[i].name != NULL; ++i) {
        if (strcmp(keys[i].name, name) == 0) {
            lua_pushinteger(L, (lua_Integer)keys[i].value);

            return 1;
        }
    }

    for (int i = 0; mouseButtons[i].name != NULL; ++i) {
        if (strcmp(mouseButtons[i].name, name) == 0) {
            lua_pushinteger(L, (lua_Integer)mouseButtons[i].value);

            return 1;
        }
    }

    lua_pushnil(L);

    return 1;
}

/* ↓ input.lookupKeyBinding(name) → code | nil; searches keys only ↓ */
static int lookupKeyBinding(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);

    for (int i = 0; keys[i].name != NULL; ++i) {
        if (strcmp(keys[i].name, name) == 0) {
            lua_pushinteger(L, (lua_Integer)keys[i].value);

            return 1;
        }
    }

    lua_pushnil(L);

    return 1;
}

/* ↓ input.isMouseDown(button) → boolean; queries the first window for mouse button state ↓ */
static int isMouseDown(lua_State *L) {
    int button = (int)luaL_checkinteger(L, 1);

    if (windowPool.count > 0 && windowPool.windows[0]->window) {
        int state = glfwGetMouseButton(windowPool.windows[0]->window, button);

        lua_pushboolean(L, state == GLFW_PRESS);
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

/* ↓ input.isShiftDown() → boolean ↓ */
static int isShiftDown(lua_State *L) {
    if (windowPool.count > 0 && windowPool.windows[0]->window) {
        GLFWwindow *win = windowPool.windows[0]->window;
        int left  = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT);
        int right = glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT);

        lua_pushboolean(L, left == GLFW_PRESS || right == GLFW_PRESS);
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

/* ↓ input.isControlDown() → boolean ↓ */
static int isControlDown(lua_State *L) {
    if (windowPool.count > 0 && windowPool.windows[0]->window) {
        GLFWwindow *win = windowPool.windows[0]->window;
        int left  = glfwGetKey(win, GLFW_KEY_LEFT_CONTROL);
        int right = glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL);

        lua_pushboolean(L, left == GLFW_PRESS || right == GLFW_PRESS);
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

/* ↓ input.getCursorPos() → x, y; returns cursor position in screen coords from first window ↓ */
static int getCursorPos(lua_State *L) {
    if (windowPool.count > 0 && windowPool.windows[0]->window) {
        double x, y;

        glfwGetCursorPos(windowPool.windows[0]->window, &x, &y);

        lua_pushnumber(L, (lua_Number)x);
        lua_pushnumber(L, (lua_Number)y);

        return 2;
    }

    lua_pushnumber(L, 0);
    lua_pushnumber(L, 0);

    return 2;
}

/* ↓ input.setCursorPos(x, y) → nil; sets cursor position on first window ↓ */
static int setCursorPos(lua_State *L) {
    double x = (double)luaL_checknumber(L, 1);
    double y = (double)luaL_checknumber(L, 2);

    if (windowPool.count > 0 && windowPool.windows[0]->window)
        glfwSetCursorPos(windowPool.windows[0]->window, x, y);

    return 0;
}

/* ↓ input.getCursorVisible() → boolean; true if cursor mode is normal (not hidden/disabled) ↓ */
static int getCursorVisible(lua_State *L) {
    if (windowPool.count > 0 && windowPool.windows[0]->window) {
        int mode = glfwGetInputMode(windowPool.windows[0]->window, GLFW_CURSOR);

        lua_pushboolean(L, mode == GLFW_CURSOR_NORMAL);
    } else {
        lua_pushboolean(L, 1);
    }

    return 1;
}

/* ↓ input.enableCursor(bool) → nil; show or hide cursor on all windows ↓ */
static int enableCursor(lua_State *L) {
    int enable = lua_toboolean(L, 1);
    int mode   = enable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN;

    for (int i = 0; i < windowPool.count; ++i) {
        if (windowPool.windows[i]->window)
            glfwSetInputMode(windowPool.windows[i]->window, GLFW_CURSOR, mode);
    }

    return 0;
}

/* ↓ input.screenToVector(x, y) → Vector; converts screen pixel coords to a normalised 2D vector ↓ */
/* ↓ result is (x/w * 2 - 1, 1 - y/h * 2) matching typical NDC conventions ↓ */
static int screenToVector(lua_State *L) {
    float sx = (float)luaL_checknumber(L, 1);
    float sy = (float)luaL_checknumber(L, 2);

    float w = 1.0f, h = 1.0f;

    if (windowPool.count > 0 && windowPool.windows[0]->window) {
        int iw, ih;

        glfwGetWindowSize(windowPool.windows[0]->window, &iw, &ih);

        if (iw > 0) w = (float)iw;
        if (ih > 0) h = (float)ih;
    }

    lua_getglobal(L, "Vector");

    if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, (lua_Number)(sx / w * 2.0f - 1.0f));
        lua_pushnumber(L, (lua_Number)(1.0f - sy / h * 2.0f));
        lua_pushnumber(L, 0);
        lua_call(L, 3, 1);

        return 1;
    }

    lua_pop(L, 1);

    lua_newtable(L);
    lua_pushnumber(L, (lua_Number)(sx / w * 2.0f - 1.0f));
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, (lua_Number)(1.0f - sy / h * 2.0f));
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "z");

    return 1;
}

/* ↓ input.selectWeapon(slot) → nil; stub — no weapon system in Vanir, provided for API completeness ↓ */
static int selectWeapon(lua_State *L) {
    (void)L;

    return 0;
}

/* ↓ input.lockControls(bool) → nil; locks/unlocks input processing globally ↓ */
static bool controlsLocked = false;

static int lockControls(lua_State *L) {
    controlsLocked = lua_toboolean(L, 1) != 0;

    return 0;
}

/* ↓ input.isControlLocked() → boolean ↓ */
static int isControlLocked(lua_State *L) {
    lua_pushboolean(L, controlsLocked ? 1 : 0);

    return 1;
}

/* ↓ input.canLockControls() → boolean; always true in Vanir ↓ */
static int canLockControls(lua_State *L) {
    lua_pushboolean(L, 1);

    return 1;
}

/* ↓ input.isGameUIVisible() → boolean; stub — no game UI layer in Vanir, always false ↓ */
static int isGameUIVisible(lua_State *L) {
    lua_pushboolean(L, 0);

    return 1;
}

/* ↓ input.getAnalogValue(code) → number; returns axis value [0, 1] for gamepad axes, 0 for digital keys ↓ */
static int getAnalogValue(lua_State *L) {
    (void)L;

    /* ↓ stub: no gamepad axis polling yet ↓ */
    lua_pushnumber(L, 0);

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

    /* ↓ input.isKeyDown — same as getKey on Win32, uses GetKeyState ↓ */
    static int isKeyDown(lua_State *L) {
        int key = (int)luaL_checkinteger(L, 1);

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
        {"getKey",           getKey},
        {"getKeyName",       getInputName},
        {"lookupBinding",    lookupBinding},
        {"lookupKeyBinding", lookupKeyBinding},
        {"isKeyDown",        isKeyDown},
        {"isMouseDown",      isMouseDown},
        {"isShiftDown",      isShiftDown},
        {"isControlDown",    isControlDown},
        {"getCursorPos",     getCursorPos},
        {"setCursorPos",     setCursorPos},
        {"getCursorVisible", getCursorVisible},
        {"screenToVector",   screenToVector},
        {"enableCursor",     enableCursor},
        {"selectWeapon",     selectWeapon},
        {"lockControls",     lockControls},
        {"isControlLocked",  isControlLocked},
        {"canLockControls",  canLockControls},
        {"isGameUIVisible",  isGameUIVisible},
        {"getAnalogValue",   getAnalogValue},

        {NULL, NULL}
    };
#else
    static void keyPressHandle(struct hook *instance, lua_State *L) { (void)instance; (void)L; }
    static void keyReleaseHandle(struct hook *instance, lua_State *L) { (void)instance; (void)L; }

    int getKey(lua_State *L) {
        int key = (int)luaL_checkinteger(L, 1);

        if (windowPool.count > 0 && windowPool.windows[0]->window) {
            int state = glfwGetKey(windowPool.windows[0]->window, key);

            lua_pushboolean(L, state == GLFW_PRESS);
        } else {
            lua_pushboolean(L, 0);
        }

        return 1;
    }

    /* ↓ input.isKeyDown — alias of getKey on non-Win32; queries first window via GLFW ↓ */
    static int isKeyDown(lua_State *L) {
        return getKey(L);
    }

    static const luaL_Reg luaInput[] = {
        {"getKey",           getKey},
        {"getKeyName",       getInputName},
        {"lookupBinding",    lookupBinding},
        {"lookupKeyBinding", lookupKeyBinding},
        {"isKeyDown",        isKeyDown},
        {"isMouseDown",      isMouseDown},
        {"isShiftDown",      isShiftDown},
        {"isControlDown",    isControlDown},
        {"getCursorPos",     getCursorPos},
        {"setCursorPos",     setCursorPos},
        {"getCursorVisible", getCursorVisible},
        {"screenToVector",   screenToVector},
        {"enableCursor",     enableCursor},
        {"selectWeapon",     selectWeapon},
        {"lockControls",     lockControls},
        {"isControlLocked",  isControlLocked},
        {"canLockControls",  canLockControls},
        {"isGameUIVisible",  isGameUIVisible},
        {"getAnalogValue",   getAnalogValue},

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
