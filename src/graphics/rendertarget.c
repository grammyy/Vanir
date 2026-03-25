#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <stdlib.h>
#include <string.h>

#include "../vanir.h"
#include "../types.h"
#include "../modules/windows.h"
#include "../modules/render.h"
#include "rendertarget.h"
#include "textures.h"
#include "shader.h"

extern struct VanirGPU gpu;
void flushBatches(struct glfwWindow *w);
extern struct windowPool windowPool;

/* ↓ proxy glfwWindow used to redirect draw calls into an RT ↓ */
static struct glfwWindow rtProxy;
/* ↓ window that was current before selectRenderTarget ↓ */
static struct glfwWindow *prevRenderWindow = NULL;

struct RenderTargetPool rtPool = {NULL, 0};

/* ↓ current render target being rendered into; NULL means rendering to the window ↓ */
static struct RenderTarget *currentRT = NULL;

/* ↓ build sampler bind group layout + bind group for sampling this RT as a texture ↓ */
static bool buildSampleBindGroup(struct RenderTarget *rt) {
    struct Texture *tex = &rt->tex;

    /* ↓ layout: binding 0 = texture, binding 1 = sampler ↓ */
    WGPUBindGroupLayoutEntry entries[2] = {0};

    entries[0].binding = 0;
    entries[0].visibility = WGPUShaderStage_Fragment;
    entries[0].texture.sampleType = WGPUTextureSampleType_Float;
    entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

    entries[1].binding = 1;
    entries[1].visibility = WGPUShaderStage_Fragment;
    entries[1].sampler.type = WGPUSamplerBindingType_Filtering;

    WGPUBindGroupLayoutDescriptor bgl_desc = {0};
    bgl_desc.entryCount = 2;
    bgl_desc.entries = entries;
    tex->bindGroupLayout = wgpuDeviceCreateBindGroupLayout(gpu.device, &bgl_desc);

    if (!tex->bindGroupLayout) {
        throw("RenderTarget", tex->name, "wgpuDeviceCreateBindGroupLayout failed");

        return false;
    }

    WGPUSamplerDescriptor samp_desc = {0};
    samp_desc.addressModeU = WGPUAddressMode_ClampToEdge;
    samp_desc.addressModeV = WGPUAddressMode_ClampToEdge;
    samp_desc.magFilter = WGPUFilterMode_Linear;
    samp_desc.minFilter = WGPUFilterMode_Linear;
    samp_desc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
    samp_desc.maxAnisotropy = 1;
    tex->sampler = wgpuDeviceCreateSampler(gpu.device, &samp_desc);

    if (!tex->sampler) {
        throw("RenderTarget", tex->name, "wgpuDeviceCreateSampler failed");

        return false;
    }

    WGPUBindGroupEntry bg_entries[2] = {0};
    bg_entries[0].binding = 0;
    bg_entries[0].textureView = tex->view;
    bg_entries[1].binding = 1;
    bg_entries[1].sampler = tex->sampler;

    WGPUBindGroupDescriptor bg_desc = {0};
    bg_desc.layout = tex->bindGroupLayout;
    bg_desc.entryCount = 2;
    bg_desc.entries = bg_entries;
    tex->bindGroup = wgpuDeviceCreateBindGroup(gpu.device, &bg_desc);

    if (!tex->bindGroup) {
        throw("RenderTarget", tex->name, "wgpuDeviceCreateBindGroup failed");

        return false;
    }

    return true;
}

/* ↓ (re)allocate the backing texture at a given size ↓ */
static bool allocTexture(struct RenderTarget *rt, uint32_t w, uint32_t h) {
    struct Texture *tex = &rt->tex;

    /* ↓ release old resources ↓ */
    if (tex->bindGroup)       { wgpuBindGroupRelease(tex->bindGroup);             tex->bindGroup       = NULL; }
    if (tex->bindGroupLayout) { wgpuBindGroupLayoutRelease(tex->bindGroupLayout); tex->bindGroupLayout = NULL; }
    if (tex->sampler)         { wgpuSamplerRelease(tex->sampler);                 tex->sampler         = NULL; }
    if (tex->view)            { wgpuTextureViewRelease(tex->view);                tex->view            = NULL; }
    if (tex->texture)         { wgpuTextureRelease(tex->texture);                 tex->texture         = NULL; }

    tex->width = w;
    tex->height = h;
    /* ↓ match the swapchain format so CopyTextureToTexture succeeds ↓ */
    tex->format = (windowPool.count > 0) ? windowPool.windows[0]->surfaceFormat : WGPUTextureFormat_BGRA8Unorm;

    WGPUTextureDescriptor tex_desc = {0};
    tex_desc.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    tex_desc.dimension = WGPUTextureDimension_2D;
    tex_desc.size.width = w;
    tex_desc.size.height = h;
    tex_desc.size.depthOrArrayLayers = 1;
    tex_desc.format = tex->format;
    tex_desc.mipLevelCount = 1;
    tex_desc.sampleCount = 1;
    tex->texture = wgpuDeviceCreateTexture(gpu.device, &tex_desc);

    if (!tex->texture) {
        throw("RenderTarget", tex->name, "wgpuDeviceCreateTexture failed");

        return false;
    }

    tex->view = wgpuTextureCreateView(tex->texture, NULL);

    if (!tex->view) {
        throw("RenderTarget", tex->name, "wgpuTextureCreateView failed");
        wgpuTextureRelease(tex->texture);

        tex->texture = NULL;

        return false;
    }

    return buildSampleBindGroup(rt);
}

/* ↓ render.createRenderTarget(name [, width, height]) → Texture ↓ */
/* ↓ loosely like Starfall's render.createRenderTarget; defaults to 512x512 ↓ */
int renderTargetCreate(lua_State *L) {
    const char *name  = luaL_checkstring(L, 1);
    uint32_t   width  = (uint32_t)luaL_optinteger(L, 2, 512);
    uint32_t   height = (uint32_t)luaL_optinteger(L, 3, 512);

    if (!gpu.device) {
        throw("RenderTarget", name, "no GPU device yet — call createWindow first");

        lua_pushnil(L);

        return 1;
    }

    if (!width || !height) {
        throw("RenderTarget", name, "width and height must be non-zero");

        lua_pushnil(L);

        return 1;
    }

    struct RenderTarget *rt = calloc(1, sizeof(struct RenderTarget));

    if (!rt) {
        throw("RenderTarget", name, "calloc failed");

        lua_pushnil(L);

        return 1;
    }

    rt->tex.name = name;

    if (!allocTexture(rt, width, height)) {
        free(rt);

        lua_pushnil(L);

        return 1;
    }

    /* ↓ grow pool ↓ */
    struct RenderTarget **temp = realloc(rtPool.targets, (rtPool.count + 1) * sizeof(struct RenderTarget *));

    if (!temp) {
        throw("RenderTarget", name, "realloc failed");
        
        if (rt->tex.bindGroup)       { wgpuBindGroupRelease(rt->tex.bindGroup);             rt->tex.bindGroup       = NULL; }
        if (rt->tex.bindGroupLayout) { wgpuBindGroupLayoutRelease(rt->tex.bindGroupLayout); rt->tex.bindGroupLayout = NULL; }
        if (rt->tex.sampler)         { wgpuSamplerRelease(rt->tex.sampler);                 rt->tex.sampler         = NULL; }
        if (rt->tex.view)            { wgpuTextureViewRelease(rt->tex.view);                rt->tex.view            = NULL; }
        if (rt->tex.texture)         { wgpuTextureRelease(rt->tex.texture);                 rt->tex.texture         = NULL; }
        
        free(rt);

        lua_pushnil(L);

        return 1;
    }

    rtPool.targets = temp;
    rtPool.targets[rtPool.count++] = rt;

    vanir_log_info("renderTargetCreate: \"%s\" %ux%u ready", name, width, height);

    pushTexture(L, &rt->tex);

    return 1;
}

/* ↓ open an off-screen render pass into this RT; draw calls will go here ↓ */
/* ↓ render.selectRenderTarget(tex) ↓ */
int renderTargetSelect(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);
    struct RenderTarget *rt = rt_from_tex(tex);

    if (rt->pending) {
        throw("selectRenderTarget", tex->name, "already selected — call stopRenderTarget first");

        return 0;
    }

    WGPUCommandEncoderDescriptor enc_desc = {0};
    rt->encoder = wgpuDeviceCreateCommandEncoder(gpu.device, &enc_desc);

    if (!rt->encoder) {
        throw("selectRenderTarget", tex->name, "wgpuDeviceCreateCommandEncoder failed");

        return 0;
    }

    WGPURenderPassColorAttachment att = {0};
    att.view = tex->view;
    att.loadOp = WGPULoadOp_Load;
    att.storeOp = WGPUStoreOp_Store;
    att.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

    WGPURenderPassDescriptor pass_desc = {0};
    pass_desc.colorAttachmentCount = 1;
    pass_desc.colorAttachments = &att;

    rt->passEncoder = wgpuCommandEncoderBeginRenderPass(rt->encoder, &pass_desc);

    if (!rt->passEncoder) {
        throw("selectRenderTarget", tex->name, "wgpuCommandEncoderBeginRenderPass failed");
        wgpuCommandEncoderRelease(rt->encoder);

        rt->encoder = NULL;

        return 0;
    }

    rt->pending = true;
    currentRT   = rt;

    /* ↓ fill the proxy glfwWindow so draw calls flow into the RT pass ↓ */
    memset(&rtProxy, 0, sizeof(rtProxy));
    
    rtProxy.frame.encoder     = rt->encoder;
    rtProxy.frame.passEncoder = rt->passEncoder;
    rtProxy.frame.view        = tex->view;
    rtProxy.frame.texture     = tex->texture;
    rtProxy.frame.pending     = true;
    rtProxy.fbWidth           = (int)tex->width;
    rtProxy.fbHeight          = (int)tex->height;
    rtProxy.width             = (int)tex->width;
    rtProxy.height            = (int)tex->height;
    rtProxy.name              = tex->name;
    rtProxy.surfaceFormat     = tex->format;

    /* ↓ borrow the pipeline from the first real window for uniform bind group ↓ */
    if (windowPool.count > 0) {
        struct glfwWindow *mainWin = windowPool.windows[0];
        rtProxy.pipeline = mainWin->pipeline;

        /* ↓ update viewport uniform to RT dimensions ↓ */
        if (mainWin->pipeline && mainWin->pipeline->uniformBuffer) {
            float vp[2] = { (float)tex->width, (float)tex->height };
            wgpuQueueWriteBuffer(gpu.queue, mainWin->pipeline->uniformBuffer, 0, vp, sizeof(vp));
        }

        /* ↓ set pipeline on the RT pass ↓ */
        if (rt->passEncoder && mainWin->pipeline->pipeline) {
            wgpuRenderPassEncoderSetPipeline(rt->passEncoder, mainWin->pipeline->pipeline);
            if (mainWin->pipeline->uniformBindGroup)
                wgpuRenderPassEncoderSetBindGroup(rt->passEncoder, 0, mainWin->pipeline->uniformBindGroup, 0, NULL);
        }
    }

    /* ↓ redirect draw calls to the proxy ↓ */
    prevRenderWindow = currentRenderWindow;
    currentRenderWindow = &rtProxy;

    vanir_log_info("selectRenderTarget: \"%s\" %ux%u open", tex->name, tex->width, tex->height);

    return 0;
}

/* ↓ finish and submit the RT render pass ↓ */
/* ↓ render.stopRenderTarget() ↓ */
int renderTargetStop(lua_State *L) {
    (void)L;

    if (!currentRT || !currentRT->pending) {
        throw("stopRenderTarget", "?", "no active render target — call selectRenderTarget first");

        return 0;
    }

    struct RenderTarget *rt = currentRT;

    /* ↓ sync proxy pass back in case draw calls swapped it ↓ */
    rt->passEncoder = rtProxy.frame.passEncoder;

    /* ↓ flush any batched geometry accumulated during RT draw calls ↓ */
    /* ↓ must happen before ending the pass or it bleeds into the next window flush ↓ */
    if (rtProxy.frame.passEncoder) {
        vanir_log_info("stopRenderTarget: \"%s\" flushing batches", rt->tex.name);
        flushBatches(&rtProxy);

        /* ↓ sync back after flushBatches may have swapped passEncoder ↓ */
        rt->passEncoder = rtProxy.frame.passEncoder;
    }

    if (rt->passEncoder) {
        wgpuRenderPassEncoderEnd(rt->passEncoder);
        wgpuRenderPassEncoderRelease(rt->passEncoder);

        rt->passEncoder = NULL;
        rtProxy.frame.passEncoder = NULL;
    }

    WGPUCommandBufferDescriptor cmd_desc = {0};
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(rt->encoder, &cmd_desc);

    if (cmd) {
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
    }

    wgpuCommandEncoderRelease(rt->encoder);

    rt->encoder = NULL;
    rtProxy.frame.encoder = NULL;
    rt->pending = false;

    /* ↓ restore previous render target and re-upload that window's viewport ↓ */
    currentRenderWindow = prevRenderWindow;
    prevRenderWindow = NULL;
    currentRT = NULL;

    if (currentRenderWindow && windowPool.count > 0) {
        struct glfwWindow *mainWin = windowPool.windows[0];

        if (mainWin->pipeline && mainWin->pipeline->uniformBuffer) {
            float vp[2] = { (float)mainWin->fbWidth, (float)mainWin->fbHeight };
            
            wgpuQueueWriteBuffer(gpu.queue, mainWin->pipeline->uniformBuffer, 0, vp, sizeof(vp));
        }
    }

    vanir_log_info("stopRenderTarget: \"%s\" submitted", rt->tex.name);

    return 0;
}

/* ↓ render.clearRenderTarget(tex [, color]) ↓ */
int renderTargetClear(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    float r = 0, g = 0, b = 0, a = 0;

    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "r"); r = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
        lua_getfield(L, 2, "g"); g = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
        lua_getfield(L, 2, "b"); b = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
        lua_getfield(L, 2, "a"); a = (float)lua_tonumber(L, -1) / 255.0f; lua_pop(L, 1);
    }

    vanir_log_info("clearRenderTarget: \"%s\" rgba=(%.2f,%.2f,%.2f,%.2f)", tex->name, r, g, b, a);

    /* ↓ open a one-shot clear pass ↓ */
    WGPUCommandEncoderDescriptor enc_desc = {0};
    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, &enc_desc);

    WGPURenderPassColorAttachment att = {0};
    att.view = tex->view;
    att.loadOp = WGPULoadOp_Clear;
    att.storeOp = WGPUStoreOp_Store;
    att.clearValue = (WGPUColor){ r, g, b, a };
    att.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

    WGPURenderPassDescriptor pass_desc = {0};
    pass_desc.colorAttachmentCount = 1;
    pass_desc.colorAttachments = &att;

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(enc, &pass_desc);

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUCommandBufferDescriptor cmd_desc = {0};
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, &cmd_desc);

    wgpuQueueSubmit(gpu.queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(enc);

    return 0;
}

/* ↓ draw an RT as a textured quad on the current window ↓ */
/* ↓ render.setRenderTargetTexture(tex, sx, sy, sw, sh, dx, dy [, dw, dh]) ↓ */
/* ↓ sx/sy/sw/sh = source region on the RT; dx/dy/dw/dh = destination rect on the window ↓ */
/* ↓ dw/dh default to sw/sh ↓ */
int renderTargetSetTexture(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.encoder) {
        throw("setRenderTargetTexture", tex->name, "no active window frame");

        return 0;
    }

    /* ↓ ensure there is an open render pass to draw into ↓ */
    if (!w->frame.passEncoder) {
        throw("setRenderTargetTexture", tex->name, "no active render pass — call selectRender first");

        return 0;
    }

    float sx = (float)luaL_optnumber(L, 2, 0);
    float sy = (float)luaL_optnumber(L, 3, 0);
    float sw = (float)luaL_optnumber(L, 4, (lua_Number)tex->width);
    float sh = (float)luaL_optnumber(L, 5, (lua_Number)tex->height);
    float dx = (float)luaL_optnumber(L, 6, 0);
    float dy = (float)luaL_optnumber(L, 7, 0);
    float dw = (float)luaL_optnumber(L, 8, sw);
    float dh = (float)luaL_optnumber(L, 9, sh);

    /* ↓ compute UV coords from source rect ↓ */
    float u0 = sx / (float)tex->width;
    float v0 = sy / (float)tex->height;
    float u1 = (sx + sw) / (float)tex->width;
    float v1 = (sy + sh) / (float)tex->height;

    vanir_log_info("setRenderTargetTexture: \"%s\" src=(%.0f,%.0f,%.0f,%.0f) dst=(%.0f,%.0f,%.0f,%.0f) uv=(%.2f,%.2f,%.2f,%.2f)", tex->name, sx, sy, sw, sh, dx, dy, dw, dh, u0, v0, u1, v1);

    drawTexturedQuadImmediate(w, tex, dx, dy, dw, dh, u0, v0, u1, v1);

    return 0;
}

void destroyAllRenderTargets(void) {
    vanir_log_info("destroyAllRenderTargets: releasing %d targets", rtPool.count);

    for (int i = 0; i < rtPool.count; ++i) {
        struct RenderTarget *rt = rtPool.targets[i];
        struct Texture *tex = &rt->tex;

        vanir_log_info("destroyAllRenderTargets: releasing \"%s\"", tex->name);

        if (rt->passEncoder)        { wgpuRenderPassEncoderRelease(rt->passEncoder);    rt->passEncoder      = NULL; }
        if (rt->encoder)            { wgpuCommandEncoderRelease(rt->encoder);           rt->encoder          = NULL; }
        if (tex->bindGroup)         { wgpuBindGroupRelease(tex->bindGroup);             tex->bindGroup       = NULL; }
        if (tex->bindGroupLayout)   { wgpuBindGroupLayoutRelease(tex->bindGroupLayout); tex->bindGroupLayout = NULL; }
        if (tex->sampler)           { wgpuSamplerRelease(tex->sampler);                 tex->sampler         = NULL; }
        if (tex->view)              { wgpuTextureViewRelease(tex->view);                tex->view            = NULL; }
        if (tex->texture)           { wgpuTextureRelease(tex->texture);                 tex->texture         = NULL; }

        free(rt);
    }

    free(rtPool.targets);

    rtPool.targets = NULL;
    rtPool.count   = 0;
}