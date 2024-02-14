#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <lua.h>

int drawLine(lua_State *L);
int drawRect(lua_State *L);
int drawCircle(lua_State *L);
int drawFilledCircle(lua_State *L);
int drawPoly(lua_State *L);
int drawVertex(lua_State *L);

#endif