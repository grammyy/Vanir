#ifndef INPUT_H
#define INPUT_H

#include <lua.h>

int inputInit(lua_State* L);

extern struct hook inputPressed;
extern struct hook inputReleased;

#endif