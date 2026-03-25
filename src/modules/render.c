/* render.c */
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <stdlib.h>

#include "../vanir.h"
#include "../types.h"
#include "windows.h"
#include "render.h"
#include "../graphics/rendertarget.h"
#include "../graphics/textures.h"
#include "../graphics/shader.h"
#include "../graphics/render.h"

extern struct VanirGPU gpu;

/* ↓ activeShader is owned by graphics/shader.c; NULL means use the built-in pipeline ↓ */
extern struct Shader *activeShader;

/* ↓ window currently targeted by draw calls. Set by selectRender, cleared by stopRender ↓ */
struct glfwWindow *currentRenderWindow = NULL;

void flushBatches(struct glfwWindow *w);

/* ↓ render passes are a contiguous block of draw commands ↓ */
/* ↓ selectRender opens one with LoadOp_Load (preserve previous contents), clear() closes and reopens with LoadOp_Clear using color ↓ */
static void beginPass(struct glfwWindow *w, WGPULoadOp loadOp) {
    WGPURenderPassColorAttachment color_att = {0};
    color_att.view = w->frame.view;
    color_att.loadOp = loadOp;
    color_att.storeOp = WGPUStoreOp_Store;
    color_att.clearValue = (WGPUColor){
        w->clearColor.r,
        w->clearColor.g,
        w->clearColor.b,
        w->clearColor.a
    };
    color_att.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

    WGPURenderPassDescriptor pass_desc = {0};
    pass_desc.colorAttachmentCount = 1;
    pass_desc.colorAttachments = &color_att;

    w->frame.passEncoder = wgpuCommandEncoderBeginRenderPass(w->frame.encoder, &pass_desc);

    struct Pipeline *p = w->pipeline;

    if (w->frame.passEncoder && p) {
        /* ↓ if a custom shader is active, use it; otherwise use the built-in pipeline ↓ */
        WGPURenderPipeline activePipeline = p->pipeline;

        if (activeShader) {
            activePipeline = activeShader->pipeline;
            vanir_log_info("beginPass: [%s] using custom shader \"%s\"", w->name, activeShader->name);
        } else {
            vanir_log_info("beginPass: [%s] using built-in pipeline", w->name);
        }

        if (activePipeline) {
            wgpuRenderPassEncoderSetPipeline(w->frame.passEncoder, activePipeline);

            if (p->uniformBindGroup)
                wgpuRenderPassEncoderSetBindGroup(w->frame.passEncoder, 0, p->uniformBindGroup, 0, NULL);
        }
    }
}

/* ↓ stopRender closes the pass, then flushBatches opens another to upload and draw cpu batch ↓ */
static void endPass(struct glfwWindow *w) {
    if (w->frame.passEncoder) {
        wgpuRenderPassEncoderEnd(w->frame.passEncoder);
        wgpuRenderPassEncoderRelease(w->frame.passEncoder);
        
        w->frame.passEncoder = NULL;
    }
}

// window methods ↓↓↓ window methods ///
int selectRender(lua_State *L) {
    struct glfwWindow **window = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
    struct glfwWindow *w = *window;

    if (w->quit) {
        vanir_log_info("selectRender: window \"%s\" is closing, skipping frame", w->name);
        
        return 0;
    }

    if (!w->surface) {
        vanir_log_info("selectRender: window \"%s\" has no surface, skipping frame", w->name);
        
        return 0;
    }

    if (w->minimized) {
        vanir_log_info("selectRender: window \"%s\" is minimized, skipping frame", w->name);
        
        return 0;
    }

    if (w->frame.pending) {
        throw("selectRender", w->name, "frame already acquired — call stopRender then update before selectRender");
        
        return 0;
    }

    vanir_log_info("selectRender: acquiring surface texture for \"%s\"", w->name);

    /* ↓ grab next texture from the swapchain ↓ */
    WGPUSurfaceTexture surf_tex = {0};
    wgpuSurfaceGetCurrentTexture(w->surface, &surf_tex);

    if (surf_tex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal && surf_tex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        throw("selectRender", w->name,
            surf_tex.status == WGPUSurfaceGetCurrentTextureStatus_Timeout ? "surface timeout" :
            surf_tex.status == WGPUSurfaceGetCurrentTextureStatus_Outdated ? "surface outdated" :
            surf_tex.status == WGPUSurfaceGetCurrentTextureStatus_Lost ? "surface lost" :
            "wgpuSurfaceGetCurrentTexture failed (unknown status)");
        
        return 0;
    }

    /* ↓ create a view into the swapchain for this frame */
    w->frame.texture = surf_tex.texture;
    w->frame.view = wgpuTextureCreateView(w->frame.texture, NULL);

    if (!w->frame.view) {
        throw("selectRender", w->name, "wgpuTextureCreateView returned NULL");
        
        return 0;
    }

    /* ↓ open command encoder; all gpu commands for this frame go through here ↓ */
    WGPUCommandEncoderDescriptor enc_desc = {0};
    w->frame.encoder = wgpuDeviceCreateCommandEncoder(gpu.device, &enc_desc);

    if (!w->frame.encoder) {
        throw("selectRender", w->name, "wgpuDeviceCreateCommandEncoder returned NULL");
        wgpuTextureViewRelease(w->frame.view);
        
        w->frame.view = NULL;
        
        return 0;
    }

    /* ↓ push updated viewport to uniform buffer ↓ */
    struct Pipeline *p = w->pipeline;
    if (p && p->uniformBuffer) {
        float vp[2] = { (float)w->fbWidth, (float)w->fbHeight };
        
        wgpuQueueWriteBuffer(gpu.queue, p->uniformBuffer, 0, vp, sizeof(vp));
    }

    /* ↓ open initial pass with LoadOp_Load; but dont clear ↓ */
    beginPass(w, WGPULoadOp_Load);

    if (!w->frame.passEncoder) {
        throw("selectRender", w->name, "wgpuCommandEncoderBeginRenderPass returned NULL");
        wgpuCommandEncoderRelease(w->frame.encoder);
        
        w->frame.encoder = NULL;
        
        wgpuTextureViewRelease(w->frame.view);
        
        w->frame.view = NULL;
        
        return 0;
    }

    vanir_log_info("selectRender: frame ready — encoder=%p  passEncoder=%p  view=%p", (void*)w->frame.encoder, (void*)w->frame.passEncoder, (void*)w->frame.view);

    currentRenderWindow = w;
    w->frame.pending = true;

    return 0;
}

int stopRender(lua_State *L) {
    struct glfwWindow **window = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
    struct glfwWindow *w = *window;

    if (!w->frame.encoder) {
        if (!w->minimized && !w->quit)
            throw("stopRender", w->name, "no active encoder — was selectRender called?");
        return 0;
    }

    vanir_log_info("stopRender: ending pass and submitting for \"%s\"", w->name);

    /* ↓ closes initial geometry pass ↓ */
    endPass(w);

    /* ↓ if there are batched draw commands, open a second pass and flush them ↓ */
    /* ↓ batched geometry is submitted in a separate pass so it draws on top of ↓ */
    struct Pipeline *p = w->pipeline;
    if (p && p->cmdCount > 0) {
        beginPass(w, WGPULoadOp_Load);
        flushBatches(w);
        endPass(w);
    }

    /* ↓ Finish encoding and submit ↓ */
    WGPUCommandBufferDescriptor cmd_desc = {0};
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(w->frame.encoder, &cmd_desc);

    if (!cmd) {
        throw("stopRender", w->name, "wgpuCommandEncoderFinish returned NULL");
        wgpuCommandEncoderRelease(w->frame.encoder);
        
        w->frame.encoder = NULL;
        
        return 0;
    }

    /* ↓ submit everything to gpu ↓ */
    wgpuQueueSubmit(gpu.queue, 1, &cmd);
    vanir_log_info("stopRender: submitted command buffer for \"%s\"", w->name);

    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(w->frame.encoder);
    w->frame.encoder = NULL;

    /* ↓ frame.view / frame.texture stay alive until update() presents the frame. ↓ */

    if (currentRenderWindow == w)
        currentRenderWindow = NULL;

    return 0;
}

/* ↓ presents completed frame, then releases texture and view; if stopRender not called: disrecards ↓ */
int update(lua_State *L) {
    struct glfwWindow **window = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
    struct glfwWindow *w = *window;

    /* ↓ called without a pending frame — silent no-op (minimized window) ↓ */
    if (!w->frame.pending)
        return 0;

    /* ↓ stopRender never called, present would submit against texture and panic so we discard ↓ */
    if (w->frame.encoder) {
        throw("update", w->name,
              "stopRender was not called before update — frame discarded");
        endPass(w);
        wgpuCommandEncoderRelease(w->frame.encoder);
        
        w->frame.encoder = NULL;
        
        if (w->frame.view)    {
            wgpuTextureViewRelease(w->frame.view);

            w->frame.view    = NULL;
        }

        if (w->frame.texture) {
            wgpuTextureRelease(w->frame.texture);

            w->frame.texture = NULL;
        }
        
        w->frame.pending = false;
        
        return 0;
    }

    /* ↓ window was destroyed between selectRender and update; surface is gone ↓ */
    if (!w->surface) {
        vanir_log_info("update: window \"%s\" already destroyed, skipping present", w->name);
        
        w->frame.pending = false;
        
        return 0;
    }

    vanir_log_info("update: presenting frame for \"%s\"", w->name);

    /* ↓ wgpu-native can panic on invalid state, so WGPU_GUARD wraps the call to only longjmp back ↓ */
    wgpuSurfacePresent(w->surface);
    /* ↑ TODO: is this correct? ↑ */

    /* ↓ release resources after presenting ↓ */
    if (w->frame.view)    {
        wgpuTextureViewRelease(w->frame.view);

        w->frame.view    = NULL;
    }

    if (w->frame.texture) {
        wgpuTextureRelease(w->frame.texture);

        w->frame.texture = NULL;
    }

    w->frame.pending = false;
    
    return 0;
}

/* ↓ releases pipeline resources for a window ↓ */
int destroy(lua_State *L) {
    struct glfwWindow **window = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
    struct glfwWindow *w = *window;

    vanir_log_info("destroy: releasing pipeline for \"%s\"", w->name);

    releaseFrame(w);
    destroyPipeline(w->pipeline);

    w->pipeline = NULL;

    return 0;
}
// window methods ↑↑↑ window methods ///

/* ↓ active draw color; set by render.setColor, read directly by all draw calls ↓ */
/* ↓ avoids 16 lua api round-trips per draw call that the old _rendercolor global table caused ↓ */
struct color activeColor = {1.0f, 1.0f, 1.0f, 1.0f};

/* ↓ active texture; set by render.setTexture, read by textured draw calls ↓ */
struct Texture *activeTexture = NULL;

/* ↓ read color from a lua table at stack index idx (r,g,b,a fields, 0-255) ↓ */
static void colorFromTable(lua_State *L, int idx, struct color *out) {
    lua_getfield(L, idx, "r"); out->r = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
    lua_getfield(L, idx, "g"); out->g = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
    lua_getfield(L, idx, "b"); out->b = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
    lua_getfield(L, idx, "a"); out->a = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
}

/* ↓ kept for any callers that still pass a color table at stack index 1 ↓ */
void getColor(lua_State *L, struct color *color) {
    colorFromTable(L, 1, color);
}

/* ↓ draw calls call this; now just a direct struct copy, zero lua overhead ↓ */
void getGlobalColor(lua_State *L, struct color *color) {
    (void)L;
    *color = activeColor;
}

int setColor(lua_State *L) {
    colorFromTable(L, 1, &activeColor);
    
    return 0;
}

int setTexture(lua_State *L) {
    if (lua_isnil(L, 1)) {
        vanir_log_info("setTexture: cleared");
        activeTexture = NULL;

        return 0;
    }

    activeTexture = getTexture(L, 1);
    vanir_log_info("setTexture: \"%s\"", activeTexture->name);

    return 0;
}
/* ↑ active color cached in C; render.setColor writes here, draw calls read directly ↑ */

int setBlend(lua_State *L) {
    int source = luaL_checkinteger(L, 1);
    int blend  = luaL_checkinteger(L, 2);
    
    // TODO(webgpu): rebuild pipeline with new blend factors.
    
    return 0;
}

int enable(lua_State *L) {
    int cap = luaL_checkinteger(L, 1);
    
    // TODO(webgpu): map cap to the appropriate pipeline field.
    
    return 0;
}

int disable(lua_State *L) {
    int cap = luaL_checkinteger(L, 1);
    
    return 0;
}

/* ↓ closes current pass and opens new one; clear color is applied as pass load action ↓ */
int clear(lua_State *L) {
    struct color color;

    getColor(L, &color);

    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.encoder)
        return 0;

    vanir_log_info("clear: [%s] rgba=(%.2f,%.2f,%.2f,%.2f)", w->name, color.r, color.g, color.b, color.a);

    w->clearColor = color;

    endPass(w);
    beginPass(w, WGPULoadOp_Clear);

    return 0;
}

/* stubs ↓↓↓ stubs */
int force(lua_State *L)       { return 0; }
int begin(lua_State *L)       { luaL_checkinteger(L, 1); return 0; }
int end(lua_State *L)         { return 0; }
int resetMatrix(lua_State *L) { return 0; }
int pushMatrix(lua_State *L)  { return 0; }
int popMatrix(lua_State *L)   { return 0; }
int scissor(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.passEncoder) return 0;

    uint32_t x  = (uint32_t)lua_tonumber(L, 1);
    uint32_t y  = (uint32_t)lua_tonumber(L, 2);
    uint32_t sw = (uint32_t)lua_tonumber(L, 3);
    uint32_t sh = (uint32_t)lua_tonumber(L, 4);

    vanir_log_info("scissor: [%s] x=%u y=%u w=%u h=%u", w->name, x, y, sw, sh);

    wgpuRenderPassEncoderSetScissorRect(w->frame.passEncoder, x, y, sw, sh);

    return 0;
}

/* ↓ set gpu viewport rect in pixel coords; resets scissor to full viewport ↓ */
int setViewport(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.passEncoder) return 0;

    float x  = (float)lua_tonumber(L, 1);
    float y  = (float)lua_tonumber(L, 2);
    float vw = (float)lua_tonumber(L, 3);
    float vh = (float)lua_tonumber(L, 4);

    vanir_log_info("setViewport: [%s] x=%.0f y=%.0f w=%.0f h=%.0f", w->name, x, y, vw, vh);

    /* ↓ minDepth / maxDepth standard NDC range ↓ */
    wgpuRenderPassEncoderSetViewport(w->frame.passEncoder, x, y, vw, vh, 0.0f, 1.0f);

    /* ↓ also update the viewport uniform so pixel->NDC conversion stays correct ↓ */
    struct Pipeline *p = w->pipeline;

    if (p && p->uniformBuffer) {
        float vp[2] = { vw, vh };

        wgpuQueueWriteBuffer(gpu.queue, p->uniformBuffer, 0, vp, sizeof(vp));
    }

    return 0;
}

/* ↓ reset viewport to the full framebuffer ↓ */
int resetViewport(lua_State *L) {
    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.passEncoder) return 0;

    vanir_log_info("resetViewport: [%s] %dx%d", w->name, w->fbWidth, w->fbHeight);

    wgpuRenderPassEncoderSetViewport(w->frame.passEncoder, 0, 0, (float)w->fbWidth, (float)w->fbHeight, 0.0f, 1.0f);

    struct Pipeline *p = w->pipeline;

    if (p && p->uniformBuffer) {
        float vp[2] = { (float)w->fbWidth, (float)w->fbHeight };

        wgpuQueueWriteBuffer(gpu.queue, p->uniformBuffer, 0, vp, sizeof(vp));
    }

    return 0;
}
int setQuality(lua_State *L) { return 0; }
/* stubs ↑↑↑ stubs */

const luaL_Reg luaRender[] = {
    /* ↓ draw calls ↓ */
    {"drawLine",         drawLine},
    {"drawRect",         drawRect},
    {"drawCircle",       drawCircle},
    {"drawFilledCircle", drawFilledCircle},
    {"drawPoly",         drawPoly},
    {"drawVertex",       drawVertex},
    {"drawTexturedRect", drawTexturedRect},
    
    /* ↓ something something, ill name this later ↓ */
    {"clear",        clear},
    {"setBlend",     setBlend},
    {"enable",       enable},
    {"disable",      disable},
    {"setColor",     setColor},
    {"setTexture",   setTexture},
    {"setQuality",   setQuality},
    {"force",        force},
    {"begin",        begin},
    {"exit",         end},
    {"scissor",      scissor},
    {"setViewport",  setViewport},
    {"resetViewport",resetViewport},
    {"resetMatrix",  resetMatrix},
    {"pushMatrix",   pushMatrix},
    {"popMatrix",    popMatrix},

    /* ↓ render targets ↓ */
    {"createRenderTarget",    renderTargetCreate},
    {"selectRenderTarget",    renderTargetSelect},
    {"stopRenderTarget",      renderTargetStop},
    {"clearRenderTarget",     renderTargetClear},
    {"setRenderTargetTexture",renderTargetSetTexture},

    /* ↓ setMaterial and setRenderTarget are aliases for setTexture ↓ */
    /* ↓ render targets return a Texture, so they work interchangeably ↓ */
    {"setMaterial",           setTexture},
    {"setRenderTarget",       setTexture},

    {NULL, NULL}
};

int renderInit(lua_State* L) {
    luaL_newlib(L, luaRender);

    return 1;
}
