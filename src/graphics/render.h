#ifndef GRAPHICS_H
#define GRAPHICS_H

// drawing ↓↓↓ drawing ///
int drawLine(lua_State *L);
int drawRect(lua_State *L);
int drawCircle(lua_State *L);
int drawFilledCircle(lua_State *L);
int drawPoly(lua_State *L);
int drawVertex(lua_State *L);
int drawRectOutline(lua_State *L);
int drawTriangle(lua_State *L);
int drawRoundedBox(lua_State *L);
int drawRoundedBoxEx(lua_State *L);
int drawTexturedRectUV(lua_State *L);
int drawTexturedTriangleUV(lua_State *L);
int drawPixelsRGB(lua_State *L);
int drawPixelsSubrectRGB(lua_State *L);
// drawing ↑↑↑ drawing ///

/* ↓ flush all batched geometry for this frame; packs tri & line verts into one gpu buffer ↓ */
void flushBatches(struct glfwWindow *w);

/* ↓ immediate-mode textured quad; defined in shader.c ↓ */
void drawTexturedQuadImmediate(struct glfwWindow *w, struct Texture *tex,
    float dx, float dy, float dw, float dh,
    float u0, float v0, float u1, float v1);

#endif