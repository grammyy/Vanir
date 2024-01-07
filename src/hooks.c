#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hooks.h"

struct hookPool pool;

struct hook think = { "think", NULL, 0, &think};

//input.c
struct hook inputPress = { "inputPress", NULL, 0, &inputPress};

void addHook(struct hook *instance, const char *name, void (*func)(lua_State*, int, struct hook *instance), int ref) {
    for (size_t i = 0; i < instance->pool; ++i) {
        if (strcmp(instance->stack[i].name, name) == 0) {
            instance->stack[i].func = func;
            instance->stack[i].ref = ref;

            return;
        }
    }

    struct stack *temp = realloc(instance->stack, (instance->pool + 1) * sizeof(struct stack));

    if (temp != NULL) {
        instance->stack = temp;
        instance->stack[instance->pool].name = strdup(name); // Use strdup to make a copy
        instance->stack[instance->pool].func = func;
        instance->stack[instance->pool].ref = ref;
        instance->pool += 1;
    } else {
        fprintf(stderr, "Hook \"%s\" errored with: Failed to add hook \"%s\". Memory allocation error.\n", instance->hookName, name);
    }
}

void runHook(struct hook *instance, lua_State *L) {
    for (size_t i = 0; i < instance->pool; ++i) {
        if (instance->stack[i].func != NULL) {
            instance->stack[i].func(L, instance->stack[i].ref, instance);
        }
    }
}

void freeHook(struct hook *instance, lua_State *L) {
    for (size_t i = 0; i < instance->pool; ++i) {
        if (instance->stack[i].ref != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, instance->stack[i].ref);
        }
    }

    free(instance->stack);
    instance->stack = NULL;
}

void luaWrapper(lua_State *L, int ref, struct hook *instance) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);

    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        printf("Hook \"%s\" errored with: %s.\n", instance->hookName, lua_tostring(L, -1));
    }

    lua_pop(L, 1);
}

struct hook* findHookByName(const struct hookPool* pool, const char* hookName) {
    for (size_t i = 0; i < pool->count; ++i) {
        if (strcmp(pool->hooks[i].hookName, hookName) == 0) {
            return &pool->hooks[i];
        }
    }

    return NULL;
}

int add(lua_State *L) {
    const char *hookName = luaL_checkstring(L, 1);
    const char *name = luaL_checkstring(L, 2);

    struct hook *instance = findHookByName(&pool, hookName);
    
    if (instance != NULL) {
        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);
            int ref = luaL_ref(L, LUA_REGISTRYINDEX);

            addHook(instance->address, name, luaWrapper, ref);
        } else {
            fprintf(stderr, "Hook \"%s\" errored with: Third argument must be a function.\n", instance->hookName);
        }
    } else {
        fprintf(stderr, "Hook \"%s\" not found.\n", hookName);
    }

    return 0;
}

int run(lua_State *L) {
    const char *hookName = luaL_checkstring(L, 1);

    struct hook *instance = findHookByName(&pool, hookName);

    runHook(instance->address, L);

    return 0;
}

const luaL_Reg luaHooks[] = {
    {"add",add},
    {"run",run},
    {NULL, NULL}
};

int hooksInit(lua_State* L) {
    luaL_newlib(L, luaHooks);

    pool.count = 2;

    pool.hooks = (struct hook *)malloc(pool.count * sizeof(struct hook));
    pool.hooks[0] = think;

    //input.c
    pool.hooks[1] = inputPress;

    // Example usage
    //addHook(&think, "examplehook1", hookfunc1, 0);

    return 1;
}