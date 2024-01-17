#ifndef INPUT_H
#define INPUT_H

#include <lua.h>

int inputInit(lua_State* L);

#define keyBitmask 0x80

extern struct hook inputPressed;
extern struct hook inputReleased;

#endif