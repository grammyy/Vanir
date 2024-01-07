#ifndef HOOKS_H
#define HOOKS_H

#include <lua.h>

int hooksInit(lua_State* L);

struct hookPool {
    struct hook *hooks;
    int count;
};

struct hook {
    const char *hookName;
    struct stack *stack;
    size_t pool;
    struct hook *address;
};

struct stack {
    const char *name;
    void (*func)(lua_State*, int, struct hook *instance);
    int ref;
};

void addHook(struct hook *instance, const char *name, void (*func)(lua_State*, int, struct hook *instance), int ref);
void runHook(struct hook *instance, lua_State *L);
void freeHook(struct hook *instance, lua_State *L);

extern struct hookPool pool;

extern struct hook think;

#endif