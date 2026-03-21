#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <stdlib.h>

#include "../vanir.h"
#include "windows.h"
#include "render.h"
#include "../graphics/render.h"

extern struct VanirGPU gpu;

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
    
    if (w->frame.passEncoder && p && p->pipeline) {
        wgpuRenderPassEncoderSetPipeline(w->frame.passEncoder, p->pipeline);
        
        if (p->uniformBindGroup)
            wgpuRenderPassEncoderSetBindGroup(w->frame.passEncoder, 0, p->uniformBindGroup, 0, NULL);
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
    
    releaseFrame(w);
    destroyPipeline(w->pipeline);
    
    w->pipeline = NULL;
    
    return 0;
}
// window methods ↑↑↑ window methods ///

/* ↓ these helpers get/set color 0-255 and converts them to 0.0-1.0 for gpu usage ↓ */
void getGlobalColor(lua_State *L, struct color *color) {
    lua_getglobal(L, "_rendercolor");
    lua_getfield(L, -1, "r"); color->r = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
    lua_getfield(L, -1, "g"); color->g = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
    lua_getfield(L, -1, "b"); color->b = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
    lua_getfield(L, -1, "a"); color->a = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 2);
}

void getColor(lua_State *L, struct color *color) {
    lua_getfield(L, 1, "r"); color->r = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
    lua_getfield(L, 1, "g"); color->g = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
    lua_getfield(L, 1, "b"); color->b = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
    lua_getfield(L, 1, "a"); color->a = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
}

int setColor(lua_State *L) {
    struct color color;
    
    getColor(L, &color);
    
    lua_getglobal(L, "_rendercolor");
    setFieldNumber(L, "r", color.r * 255.0f);
    setFieldNumber(L, "g", color.g * 255.0f);
    setFieldNumber(L, "b", color.b * 255.0f);
    setFieldNumber(L, "a", color.a * 255.0f);
    
    return 0;
}
/* ↑ these helpers get/set color 0-255 and converts them to 0.0-1.0 for gpu usage ↑ */

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
    // TODO(webgpu): wgpuRenderPassEncoderSetScissorRect
    (void)lua_tonumber(L, 1); (void)lua_tonumber(L, 2);
    (void)lua_tonumber(L, 3); (void)lua_tonumber(L, 4);
    
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
    
    /* ↓ something something, ill name this later ↓ */
    {"clear",       clear},
    {"setBlend",    setBlend},
    {"enable",      enable},
    {"disable",     disable},
    {"setColor",    setColor},
    {"setQuality",  setQuality},
    {"force",       force},
    {"begin",       begin},
    {"exit",        end},
    {"scissor",     scissor},
    {"resetMatrix", resetMatrix},
    {"pushMatrix",  pushMatrix},
    {"popMatrix",   popMatrix},

    {NULL, NULL}
};

int renderInit(lua_State* L) {
    luaL_newlib(L, luaRender);

    return 1;
}