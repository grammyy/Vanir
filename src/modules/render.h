#ifndef RENDER
#define RENDER

#include "../vanir.h"

int renderInit(lua_State* L);

/* window methods ↓↓↓ window methods */
int selectRender(lua_State *L);
int stopRender(lua_State *L);
int update(lua_State *L);
int setQuality(lua_State *L);
int setBlend(lua_State *L);
int enable(lua_State *L);
int disable(lua_State *L);
int setViewport(lua_State *L);
int resetViewport(lua_State *L);
/* window methods ↑↑↑ window methods */

/* ↓ clear variants ↓ */
int clearDepth(lua_State *L);
int clearRGBA(lua_State *L);

/* ↓ state/util ↓ */
int depthRange(lua_State *L);
int enableDepth(lua_State *L);
int pushCustomClipPlane(lua_State *L);
int popCustomClipPlane(lua_State *L);
int renderViewsLeft(lua_State *L);
int readPixel(lua_State *L);
int renderTargetExists(lua_State *L);
int destroyRenderTarget(lua_State *L);

/* ↓ active color; written by render.setColor, read directly by draw calls ↓ */
extern struct color activeColor;

/* ↓ active texture; written by render.setTexture, read by textured draw calls ↓ */
extern struct Texture *activeTexture;

extern struct glfwWindow *currentRenderWindow;

/* ↓ apply the active matrix transform to a vertex position in-place ↓ */
void applyActiveMatrix(float *x, float *y);

#endif