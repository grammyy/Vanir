#ifndef GRAPHICS_H
#define GRAPHICS_H

// drawing ↓↓↓ drawing ///
int drawLine(lua_State *L);
int drawRect(lua_State *L);
int drawCircle(lua_State *L);
int drawFilledCircle(lua_State *L);
int drawPoly(lua_State *L);
int drawVertex(lua_State *L);
// drawing ↑↑↑ drawing ///

/* ↓ flush all batched geometry for this frame; packs tri & line verts into one gpu buffer ↓ */
void flushBatches(struct glfwWindow *w);

#endif