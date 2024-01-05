#ifndef HOOKS_H
#define HOOKS_H

#include <lua.h>

int hooksInit(lua_State* L);

struct hooks {
    const char *name;
    void (*func)(lua_State*, int);
    int ref;  // Store Lua function reference
};

struct hook {
    const char *hookName;
    struct hooks *hooks;
    size_t pool;
};

void addHook(struct hook *instance, const char *name, void (*func)(lua_State*, int), int ref);
void runHook(const struct hook *instance, lua_State *L);
void freeHook(struct hook *instance, lua_State *L);

extern struct hook think;

#endif