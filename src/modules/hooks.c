#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../vanir.h"
#include "hooks.h"
#include "timer.h"

struct hookPool hookPool = {NULL, 0};

struct hook think   = { "think",   NULL, 0, &think,   NULL, hook_update};
struct hook onError = { "onError", NULL, 0, &onError, NULL, hook_idle};

/* ↓ allocate a new callback with given data size and type; carries typed value from C -> lua when fired ↓ */
struct callbacks* createCallback(size_t dataSize, enum dataType dataType) {
    struct callbacks* callback = malloc(sizeof(struct callbacks));
    
    if (callback) {
        callback->dataSize = dataSize;
        callback->data = malloc(dataSize);
        callback->dataType = dataType;
    } else {
        throw("Callback", "?", "Memory allocation error");
    }

    return callback;
}

/* helper functions ↓↓↓ helper functions */
void setCallback(struct callbacks* callback, const void* data) {
    memcpy(callback->data, data, callback->dataSize); //copies data into existing callback
}

void* getCallback(const struct callbacks* callback) {
    return callback->data;
}
/* helper functions ↑↑↑ helper functions */

/* ↓ fire the onError hook with a formatted message string; called by throwError below ↓ */
void fireError(const char *message) {
    if (!onError.callback) {
        onError.callback = createCallback(strlen(message) + 1, string);
    }

    setCallback(onError.callback, message);
    
    onError.status = hook_awaiting;
}

/* ↓ append hook to global pool; called once per hook during module init ↓ */
void registerHook(struct hook hookData) {
    struct hook* temp = (struct hook*)realloc(hookPool.hooks, (hookPool.count + 1) * sizeof(struct hook));

    if (temp) {
        hookPool.hooks = temp;
        hookPool.hooks[hookPool.count] = hookData;
        hookPool.count += 1;
    } else {
        throw("Hook", "pool", "Memory allocation error");
    }
}

/* ↓ listener stack; each hook has a stack of named listeners and main C handle ↓ */
void addHook(struct hook *instance, const char *name, void (*func)(lua_State*, struct hook *instance, int, struct callbacks* callback), int ref) {
    for (size_t i = 0; i < instance->pool; ++i) {
        if (strcmp(instance->stack[i].name, name) == 0) {
            instance->stack[i].func = func;
            instance->stack[i].ref = ref;

            return;
        }
    }
    /* ↑ replace existing listener with the same name instead of appending ↑ */

    /* ↓ name not found, grow and append stack ↓ */
    struct stack *temp = realloc(instance->stack, (instance->pool + 1) * sizeof(struct stack));

    if (temp) {
        instance->stack = temp;
        instance->stack[instance->pool].name = strdup(name);
        instance->stack[instance->pool].func = func;
        instance->stack[instance->pool].ref = ref;
        instance->pool += 1;
    } else {
        throw("Hook", instance->hookName, "Memory allocation error");
    }
}

/* ↓ frees the name, unrefs lua, then shifts remaining listeners down and shrinks the stack allocation ↓ */
void removeHook(struct hook *instance, const char *name, lua_State *L) {
    for (size_t i = 0; i < instance->pool; ++i) {
        if (strcmp(instance->stack[i].name, name) == 0) {
            free((void*)instance->stack[i].name);

            /* ↓ release the lua reg ref so the gc can collect the function ↓ */
            if (instance->stack[i].ref != LUA_NOREF) 
                luaL_unref(L, LUA_REGISTRYINDEX, instance->stack[i].ref);

            for (size_t j = i; j < instance->pool - 1; ++j) 
                instance->stack[j] = instance->stack[j + 1];

            struct stack *temp = realloc(instance->stack, (instance->pool - 1) * sizeof(struct stack));

            if (temp || instance->pool - 1 == 0) {
                instance->stack = temp;
                instance->pool -= 1;
            } else {
                throw("Hook", instance->hookName, "Memory allocation error");
            }

            return;
        }
    }

    char temp[strlen(name) + 12];

    snprintf(temp, sizeof(temp), "'%s' not found", name);

    throw("Hook", instance->hookName, temp);
}

/* ↓ singular hook call; calls C handle first, then every lua listener attached ↓ */
void runHook(struct hook *instance, lua_State *L) {
    if (!instance || !L) {
        throw("Hook", "?", "Failed to get hook instance");

        return;
    }

    /* ↓ main hook C handle ↓ */
    if (instance->handle) {
        instance->handle(instance, L);
    }

    for (size_t i = 0; i < instance->pool; ++i) {
        if (instance->stack[i].func) {
            if (instance->status != hook_idle) 
                instance->stack[i].func(L, instance, i, instance->callback);

                /* ↑ callbacks are done here ↑ */
        } else {
            throw("Hook", instance->hookName, "Could not find function reference");
        }
    }

    /* ↓ hook_awaiting hook types are one-shot; they fire once before going idle ↓ */
    if (instance->status == hook_awaiting) {
        instance->status = hook_idle;
    }
}

/* ↓ frees all listeners from a hook but keeps it registered ↓ */
void freeHook(struct hook *instance, lua_State *L) {
    for (size_t i = 0; i < instance->pool; ++i) {
        free((void*)instance->stack[i].name);

        if (instance->stack[i].ref != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, instance->stack[i].ref);
        }
    }

    free(instance->stack);

    instance->stack = NULL;
    instance->pool = 0;
}

/* ↓ pushed as the msgh argument to lua_pcall; baking full stack track using debug.traceback() ↓ */
static int errorHandler(lua_State *L) {
    const char *msg = lua_tostring(L, 1);

    if (!msg) {
        lua_getglobal(L, "tostring");
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);
        /* ↑ non-string error: coerce to string with tostring() ↑ */

        msg = lua_tostring(L, -1);

        lua_pop(L, 1);
    }

    /* ↓ debug.traceback(msg, 2); skips this handler frame itself ↓ */
    luaL_traceback(L, L, msg, 2);

    return 1; // leave the traceback string on the stack
}

/* ↓ C trampoline that looks up the function by registry ref and pushes the callback arg ↓ */
void luaFunc(lua_State *L, struct hook *instance, int index, struct callbacks* callback) {
    int base = lua_gettop(L);
    
    lua_pushcfunction(L, errorHandler);   // stack: ... | msgh
    
    // ↓ lua_pcall's msgh index must be absolute ↓
    int msgh = base + 1;

    /* ↓ push the lua function from the registry ↓ */
    lua_rawgeti(L, LUA_REGISTRYINDEX, instance->stack[index].ref);

    int nargs = 0;
    
    if (callback && callback->data) {
        switch (callback->dataType) {
            case number:
                lua_pushnumber(L, *(double*)callback->data); break;
            case string:
                lua_pushstring(L, (const char*)callback->data); break;
            case integer:
                lua_pushinteger(L, *(int*)callback->data); break;
            case lua_bool:
                lua_pushboolean(L, *(int*)callback->data); break;
            case function:
                lua_rawgeti(L, LUA_REGISTRYINDEX, *(int*)callback->data); break;
            default:
                lua_pushnil(L); break;
        }

        nargs = 1;
    }

    if (lua_pcall(L, nargs, LUA_MULTRET, msgh) != LUA_OK) {
        throw("Hook", instance->hookName, lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    /* ↓ remove the message handler from the stack ↓ */
    lua_remove(L, msgh);
}

/* ↓ find a hook in the global pool by name; returns NULL if not found ↓ */
struct hook* findHook(const char* hookName) {
    for (size_t i = 0; i < hookPool.count; ++i) {
        if (strcmp(hookPool.hooks[i].hookName, hookName) == 0) {
            return &hookPool.hooks[i];
        }
    }
    
    return NULL;
}

/* ↓ lua interface for internal C functions ↓ */
int luaAdd(lua_State *L) {
    const char *hookName = luaL_checkstring(L, 1);
    const char *name = luaL_checkstring(L, 2);

    struct hook *instance = findHook(hookName);
    
    if (instance) {
        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);

            int ref = luaL_ref(L, LUA_REGISTRYINDEX);

            addHook(instance->address, name, luaFunc, ref);
        } else {
            throw("Hook", instance->hookName, "Third argument must be a function");
        }
    } else {
        throw("Hook", hookName, "Not found");
    }

    return 0;
}

int luaRemove(lua_State *L) {
    const char *hookName = luaL_checkstring(L, 1);
    const char *name = luaL_checkstring(L, 2);

    struct hook *instance = findHook(hookName);
    
    if (instance) {
        removeHook(instance->address, name, L);
    } else {
        throw("Hook", hookName, "Not found");
    }

    return 0;
}

int luaFree(lua_State *L) {
    const char *hookName = luaL_checkstring(L, 1);
    
    struct hook *instance = findHook(hookName);

    if (instance) {
        freeHook(instance->address, L);
    } else {
        throw("Hook", hookName, "Not found");
    }

    return 0;
}

int hooksRun(lua_State *L) {
    tickTimers(L);
    /* ↑ advance all named timers each frame ↑ */

    for (size_t i = 0; i < hookPool.count; ++i) {
        runHook(hookPool.hooks[i].address, L);
    }

    return 0;
}
/* ↑ lua interface for internal C functions ↑ */

const luaL_Reg luaHooks[] = {
    {"add",    luaAdd},
    {"remove", luaRemove},
    {"free",   luaFree},

    {NULL, NULL}
};

int hooksInit(lua_State* L) {
    luaL_newlib(L, luaHooks);

    registerHook(think);
    registerHook(onError);

    return 1;
}