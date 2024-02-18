#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../vanir.h"
#include "hooks.h"

struct hookPool pool = {NULL, 0};

struct hook think = { "think", NULL, 0, &think, NULL, hook_update};

struct callbacks* createCallback(size_t dataSize, enum dataType dataType) {
    struct callbacks* callback = malloc(sizeof(struct callbacks));
    
    callback->dataSize = dataSize;
    callback->data = malloc(dataSize);
    callback->dataType = dataType;

    // If the type is CALLBACK_TYPE_TABLE, initialize the table
    //if (dataType == function) {
    //    lua_newtable(L);
    //    lua_rawseti(L, -2, 1);  // Set the table to the data field
    //}

    return callback;
}

void setCallback(struct callbacks* callback, const void* data) {
memcpy(callback->data, data, callback->dataSize);
}

void* getCallback(const struct callbacks* callback) {
    return callback->data;
}

void registerHook(struct hookPool* pool, struct hook hookData) {
    struct hook* temp = (struct hook*)realloc(pool->hooks, (pool->count + 1) * sizeof(struct hook));

    if (temp != NULL) {
        pool->hooks = temp;
        pool->hooks[pool->count] = hookData;
        pool->count += 1;
    } else {
        throw("Hook", "pool", "Memory allocation error");
    }
}

void addHook(struct hook *instance, const char *name, void (*func)(lua_State*, struct hook *instance, int, struct callbacks* callback), int ref) {
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
        instance->stack[instance->pool].name = strdup(name);
        instance->stack[instance->pool].func = func;
        instance->stack[instance->pool].ref = ref;
        instance->pool += 1;
    } else {
        throw("Hook", instance->hookName, "Memory allocation error");
    }
}

void removeHook(struct hook *instance, const char *name) {
    for (size_t i = 0; i < instance->pool; ++i) {
        if (strcmp(instance->stack[i].name, name) == 0) {
            for (size_t j = i; j < instance->pool - 1; ++j) {
                instance->stack[j] = instance->stack[j + 1];
            }

            struct stack *temp = malloc((instance->pool - 1) * sizeof(struct stack));

            if (temp != NULL) {
                memcpy(temp, instance->stack, i * sizeof(struct stack));
                memcpy(temp + i, instance->stack + i + 1, (instance->pool - i - 1) * sizeof(struct stack));

                free(instance->stack);

                instance->stack = temp;
                instance->pool -= 1;
            } else {
                throw("Hook", instance->hookName, "Memory allocation error");
            }

            return;
        }
    }

    char temp[strlen(name) + 12];

    strcpy(temp, "'");
    strcpy(temp, name);
    strcat(temp, "' not found");

    throw("Hook", instance->hookName, temp);
}


void runHook(struct hook *instance, lua_State *L) {
    if (!instance || !L) {
        throw("Hook", "?", "Failed to get hook instance");

        return;
    }

    if (instance->handle != NULL) {
        instance->handle(instance, L);
    }

    for (size_t i = 0; i < instance->pool; ++i) {
        if (instance->stack[i].func != NULL) {
            if (instance->status!=hook_idle) {
                instance->stack[i].func(L, instance, i, instance->callback);
            }
        } else {
            throw("Hook", instance->hookName, "Could not find function reference");
        }
    }

    if (instance->status==hook_awaiting) {
        instance->status=hook_idle;
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

void luaWrapper(lua_State *L, struct hook *instance, int index, struct callbacks* callback) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, instance->stack[index].ref);

    if (callback!=NULL && callback->data != NULL) {
        for (size_t i = 0; i < callback->dataSize; ++i) {
            void* value = ((char*)callback->data) + (i * callback->dataSize);

            switch (callback->dataType) {
                case number:
                    lua_pushnumber(L, *(double*)value);
                    break;
                case string:
                    lua_pushstring(L, (const char*)value);
                    break;
                case integer:
                    lua_pushinteger(L, *(int*)value);
                    break;
                case lua_bool:
                    lua_pushboolean(L, *(int*)value);
                    break;
                case function:
                    lua_rawgeti(L, LUA_REGISTRYINDEX, *((int*)value));
                    break;
                default:
                    lua_pushnil(L);
                    break;
            }
        }
    }

    if (lua_pcall(L, callback ? callback->dataSize : 0, LUA_MULTRET, 0) != LUA_OK) {
        throw("Hook", instance->hookName, lua_tostring(L, -1));
    
        lua_pop(L, 1);
        
        return;
    }
}

struct hook* findHookByName(const struct hookPool* pool, const char* hookName) {
    for (size_t i = 0; i < pool->count; ++i) {
        if (strcmp(pool->hooks[i].hookName, hookName) == 0) {
            return &pool->hooks[i];
        }
    }
    
    return NULL;
}

int luaAdd(lua_State *L) {
    const char *hookName = luaL_checkstring(L, 1);
    const char *name = luaL_checkstring(L, 2);

    struct hook *instance = findHookByName(&pool, hookName);
    
    if (instance != NULL) {
        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);
            int ref = luaL_ref(L, LUA_REGISTRYINDEX);

            addHook(instance->address, name, luaWrapper, ref);
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

    struct hook *instance = findHookByName(&pool, hookName);
    
    if (instance != NULL) {
        removeHook(instance->address, name);
    } else {
        throw("Hook", hookName, "Not found");
    }

    return 0;
}

int luaRun(lua_State *L) {
    for (size_t i = 0; i < pool.count; ++i) {
        runHook(pool.hooks[i].address, L);
    }

    return 0;
}

const luaL_Reg luaHooks[] = {
    {"add",luaAdd},
    {"remove",luaRemove},
    {"run",luaRun},
    {NULL, NULL}
};

int hooksInit(lua_State* L) {
    luaL_newlib(L, luaHooks);

    registerHook(&pool, think);

    // Example usage for using in c
    //addHook(&think, "examplehook1", hookfunc1, 0);

    return 1;
}