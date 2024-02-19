#ifndef INPUT
#define INPUT

#include <lua.h>

int inputInit(lua_State* L);

#define keyBitmask 0x80

int getKey(lua_State *L);

extern struct hook inputPressed;
extern struct hook inputReleased;

#endif