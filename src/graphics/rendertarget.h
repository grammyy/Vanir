#ifndef GRAPHICS_RENDERTARGET_H
#define GRAPHICS_RENDERTARGET_H

#include <webgpu/webgpu.h>
#include <stdbool.h>
#include "../vanir.h"
#include "textures.h"

#define VANIR_MAX_RENDER_TARGETS 64

/* ↓ an off-screen render target; can be drawn into and sampled as a texture ↓ */
/* ↓ tex is a full Texture — pass it to render.setTexture, render.setMaterial, etc. ↓ */
struct RenderTarget {
    struct Texture   tex;              // ↓ must be first; Lua receives &rt->tex directly ↓

    // ↓ encoder / pass for off-screen rendering into this target ↓
    WGPUCommandEncoder    encoder;
    WGPURenderPassEncoder passEncoder;

    bool     pending;          // ↓ true between selectRenderTarget / stopRenderTarget ↓
};

/* ↓ global pool ↓ */
struct RenderTargetPool {
    struct RenderTarget **targets;
    int count;
};

extern struct RenderTargetPool rtPool;

/* ↓ recover the RenderTarget from a Texture pointer (tex is always the first field) ↓ */
#define rt_from_tex(t) ((struct RenderTarget *)(t))

/* ↓ called from render.c module functions ↓ */
int renderTargetCreate(lua_State *L);
int renderTargetSelect(lua_State *L);
int renderTargetStop(lua_State *L);
int renderTargetSetTexture(lua_State *L);
int renderTargetClear(lua_State *L);

/* ↓ destroy all targets; called on quit ↓ */
void destroyAllRenderTargets(void);

#endif
