#ifndef RENDER_H
#define RENDER_H

#include <lua.h>

int renderInit(lua_State* L);

// window methods ↓↓↓ window methods ///
int selectRender(lua_State *L);
int stopRender(lua_State *L);
int update(lua_State *L);
// window methods ↑↑↑ window methods ///

#endif