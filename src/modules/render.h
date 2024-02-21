#ifndef RENDER
#define RENDER

#include "../vanir.h"

int renderInit(lua_State* L);

// window methods ↓↓↓ window methods ///
int selectRender(lua_State *L);
int stopRender(lua_State *L);
int update(lua_State *L);
int setQuality(lua_State *L);
int setBlend(lua_State *L);
int enable(lua_State *L);
int disable(lua_State *L);
// window methods ↑↑↑ window methods ///

void getGlobalColor(lua_State *L, struct color *color);
void getColor(lua_State *L, struct color *color);

#endif