#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hooks.h"

struct hook think = { "think", NULL, 0};

void addHook(struct hook *instance, const char *name, void (*func)(lua_State*, int), int ref) {
    for (size_t i = 0; i < instance->pool; ++i) {
        if (strcmp(instance->hooks[i].name, name) == 0) {
            instance->hooks[i].func = func;
            instance->hooks[i].ref = ref;

            return;
        }
    }

    struct hooks *temp = realloc(instance->hooks, (instance->pool + 1) * sizeof(struct hooks));

    if (temp != NULL) {
        instance->hooks = temp;
        instance->hooks[instance->pool].name = name;
        instance->hooks[instance->pool].func = func;
        instance->hooks[instance->pool].ref = ref;
        instance->pool += 1;
    } else {
        printf("Hook \"%s\" errored with: Failed to add hook \"%s\". Memory allocation error.\n", instance->hookName, name);
    }
}

void runHook(const struct hook *instance, lua_State *L) {
    for (size_t i = 0; i < instance->pool; ++i) {
        if (instance->hooks[i].func != NULL) {
            instance->hooks[i].func(instance->hooks[i].name, L, instance->hooks[i].ref);  // Adjust parameters as needed
        }
    }
}

void freeHook(struct hook *instance, lua_State *L) {
    for (size_t i = 0; i < instance->pool; ++i) {
        if (instance->hooks[i].ref != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, instance->hooks[i].ref);
        }
    }

    free(instance->hooks);
    instance->hooks = NULL;
}

void luaFunctionCaller(const char *name, lua_State *L, int ref) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);

    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        //printf("Hook \"%s\" errored with: %s.\n", name, lua_tostring(L, -1));
    }

    lua_pop(L, 1);
}

struct hook *findHookByName(const char *name) {
    // Implement your logic to find the hook by name
    // Return a pointer to the hook if found, or NULL otherwise
    if (strcmp(name, "think") == 0) {
        return &think;
    }

    return NULL;
}

int add(lua_State *L) {
    const char *hookName = luaL_checkstring(L, 1); // First argument is the hook name
    const char *name = luaL_checkstring(L, 2);    // Second argument is the hook function name

    // Find the hook struct by name
    struct hook *instance = findHookByName(hookName);

    if (instance != NULL) {
        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);
            int ref = luaL_ref(L, LUA_REGISTRYINDEX);

            addHook(instance, name, luaFunctionCaller, ref);
        } else {
            fprintf(stderr, "Hook \"%s\" errored with: Third argument must be a function.\n", instance->hookName);
        }
    } else {
        fprintf(stderr, "Hook \"%s\" not found.\n", hookName);
    }

    return 0;
}

int run(lua_State *L) {
    runHook(&think, L);

    return 0;
}

const luaL_Reg luaHooks[] = {
    {"add",add},
    {"run",run},
    {NULL, NULL}
};

int hooksInit(lua_State* L) {
    luaL_newlib(L, luaHooks);

    // Example usage
    //addHook(&think, "examplehook1", hookfunc1, 0);

    return 1;
}