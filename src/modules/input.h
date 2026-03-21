#ifndef VANIR_INPUT_H
#define VANIR_INPUT_H

int inputInit(lua_State* L);

#define keyBitmask 0x80

int getKey(lua_State *L);

extern struct hook inputPressed;
extern struct hook inputReleased;

#endif