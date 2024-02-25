#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <lua.h>

// drawing ↓↓↓ drawing ///
int drawLine(lua_State *L);
int drawRect(lua_State *L);
int drawCircle(lua_State *L);
int drawFilledCircle(lua_State *L);
int drawPoly(lua_State *L);
int drawVertex(lua_State *L);
// drawing ↑↑↑ drawing ///

// textures ↓↓↓ textures ///
int selectTexture(lua_State *L);
int newTexture(lua_State *L);
int destroyTexture(lua_State *L);
// textures ↑↑↑ textures ///

#endif