#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../vanir.h"
#include "windows.h"
#include "hooks.h"
#include "render.h"

struct windowPool windowPool = {NULL, 0};
struct VanirGPU   gpu        = {0};

struct hook onHoverChange = { "onHoverChange", NULL, 0, &onHoverChange, NULL, hook_idle};
struct hook onFocusChange = { "onFocusChange", NULL, 0, &onFocusChange, NULL, hook_idle};
struct hook onResize      = { "onResize",      NULL, 0, &onResize,      NULL, hook_idle};
struct hook onEvent       = { "onEvent",       NULL, 0, &onEvent,       NULL, hook_idle};
struct hook onClose       = { "onClose",       NULL, 0, &onClose,       NULL, hook_idle};
struct hook onOpen        = { "onOpen",        NULL, 0, &onOpen,        NULL, hook_idle};

/* ↓ webgpu async callbacks ↓ */
static void on_adapter(WGPURequestAdapterStatus status, WGPUAdapter a, WGPUStringView msg, void *ud1, void *ud2) {
    if (status == WGPURequestAdapterStatus_Success) *(WGPUAdapter *)ud1 = a; //TODO: maybe change this later
}

static void on_device(WGPURequestDeviceStatus status, WGPUDevice d, WGPUStringView msg, void *ud1, void *ud2) {
    if (status == WGPURequestDeviceStatus_Success) *(WGPUDevice *)ud1 = d;
}
/* ↑ webgpu async callbacks ↑ */

/* ↓ shared gpu init not used; however, here for explicit init if needed ↓ */
bool vanirGPUInit(WGPUSurface firstSurface) {
    WGPUInstanceDescriptor inst_desc = {0};
    gpu.instance = wgpuCreateInstance(&inst_desc);

    if (!gpu.instance) {
        throw("GPU", "init", "wgpuCreateInstance failed");

        return false;
    }

    WGPURequestAdapterOptions adapter_opts = {0};
    adapter_opts.compatibleSurface = firstSurface;
    WGPURequestAdapterCallbackInfo adapter_cb = {0};
    adapter_cb.callback  = on_adapter;
    adapter_cb.userdata1 = &gpu.adapter;

    wgpuInstanceRequestAdapter(gpu.instance, &adapter_opts, adapter_cb);

    if (!gpu.adapter) {
        throw("GPU", "init", "wgpuInstanceRequestAdapter failed");
        wgpuInstanceRelease(gpu.instance);

        gpu.instance = NULL;

        return false;
    }

    WGPUDeviceDescriptor device_desc = {0};
    WGPURequestDeviceCallbackInfo device_cb = {0};
    device_cb.callback = on_device;
    device_cb.userdata1 = &gpu.device;

    wgpuAdapterRequestDevice(gpu.adapter, &device_desc, device_cb);

    if (!gpu.device) {
        throw("GPU", "init", "wgpuAdapterRequestDevice failed");
        wgpuAdapterRelease(gpu.adapter);
        wgpuInstanceRelease(gpu.instance);

        gpu.adapter = NULL;
        gpu.instance = NULL;

        return false;
    }

    gpu.queue = wgpuDeviceGetQueue(gpu.device);

    vanir_log_info("vanirGPUInit: shared device + queue ready");

    return true;
}

/* ↓ release shared gpu context resources; called automatnically when all windows close ↓ */
void vanirGPUDestroy(void) {
    if (gpu.queue) { 
        wgpuQueueRelease(gpu.queue);
        
        gpu.queue = NULL;
    }

    if (gpu.device) { 
        wgpuDeviceRelease(gpu.device);
        
        gpu.device = NULL;
    }

    if (gpu.adapter) { 
        wgpuAdapterRelease(gpu.adapter);
        
        gpu.adapter = NULL;
    }

    if (gpu.instance) { 
        wgpuInstanceRelease(gpu.instance);
        
        gpu.instance = NULL;
    }
    /* ↑ perhaps make a marco for these checks later ↑ */

    vanir_log_info("vanirGPUDestroy: shared GPU context released");
}

/* glfw callbacks ↓↓↓ glfw callbacks */
static struct glfwWindow *findWindow(GLFWwindow *win) {
    for (size_t i = 0; i < windowPool.count; ++i)
        if (windowPool.windows[i]->window == win)
            return windowPool.windows[i];

    return NULL;
}

static void cbResize(GLFWwindow *win, int width, int height) {
    struct glfwWindow *w = findWindow(win);

    if (!w) return;

    w->fbWidth = width;
    w->fbHeight = height;

    glfwGetWindowSize(win, &w->width, &w->height);

    onResize.status = hook_awaiting;

    if (width == 0 || height == 0) {
        w->minimized = true;

        vanir_log_info("cbResize: window \"%s\" minimized", w->name);

        return;
    }

    w->minimized = false;

    releaseFrame(w);

    WGPUSurfaceConfiguration surf_cfg = {0};
    surf_cfg.device = gpu.device;
    surf_cfg.format = w->surfaceFormat;
    surf_cfg.usage = WGPUTextureUsage_RenderAttachment;
    surf_cfg.width = (uint32_t)w->fbWidth;
    surf_cfg.height = (uint32_t)w->fbHeight;
    surf_cfg.presentMode = w->presentMode;

    wgpuSurfaceConfigure(w->surface, &surf_cfg);
}

static void cbCursorEnter(GLFWwindow *win, int entered) {
    struct glfwWindow *w = findWindow(win);

    if (!w) 
        return;

    w->hovering = (bool)entered;
    onHoverChange.status = hook_awaiting;

    setCallback(onHoverChange.callback, &(bool){entered});
}

static void cbFocus(GLFWwindow *win, int focused) {
    struct glfwWindow *w = findWindow(win);

    if (!w) 
        return;

    w->focused = (bool)focused;
    onFocusChange.status = hook_awaiting;

    setCallback(onFocusChange.callback, &(bool){focused});
}

/* ↓ marks for deferred free; cleanup happens next frame ↓ */
static void cbClose(GLFWwindow *win) {
    for (size_t i = 0; i < windowPool.count; ++i) {
        if (windowPool.windows[i]->window != win) 
            continue;
        
        windowPool.windows[i]->quit = true;
        onClose.status = hook_awaiting;
        
        vanir_log_info("cbClose: window \"%s\" marked for deferred close", windowPool.windows[i]->name);
        
        return;
    }
}
/* glfw callbacks ↑↑↑ glfw callbacks */

/* ↓ frame release ↓ */
void releaseFrame(struct glfwWindow *w) {
    if (w->frame.passEncoder) {
        wgpuRenderPassEncoderEnd(w->frame.passEncoder);
        wgpuRenderPassEncoderRelease(w->frame.passEncoder);

        w->frame.passEncoder = NULL;
    }

    if (w->frame.encoder) {
        wgpuCommandEncoderRelease(w->frame.encoder);

        w->frame.encoder = NULL;
    }

    if (w->frame.view) { 
        wgpuTextureViewRelease(w->frame.view);
        
        w->frame.view = NULL;
    }

    if (w->frame.texture) { 
        wgpuTextureRelease(w->frame.texture);
        
        w->frame.texture = NULL;
    }
    
    w->frame.pending = false;
}
/* ↑ frame release ↑ */

/* ↓ pipeline lifecycle ↓ */
void destroyPipeline(struct Pipeline *p) {
    if (!p) return;
    if (p->pipelineLine)           { wgpuRenderPipelineRelease(p->pipelineLine);            p->pipelineLine           = NULL; }
    if (p->pipeline)               { wgpuRenderPipelineRelease(p->pipeline);                p->pipeline               = NULL; }
    if (p->uniformBindGroup)       { wgpuBindGroupRelease(p->uniformBindGroup);             p->uniformBindGroup       = NULL; }
    if (p->uniformBindGroupLayout) { wgpuBindGroupLayoutRelease(p->uniformBindGroupLayout); p->uniformBindGroupLayout = NULL; }
    if (p->uniformBuffer)          { wgpuBufferRelease(p->uniformBuffer);                   p->uniformBuffer          = NULL; }
    if (p->vertexBuffer)           { wgpuBufferRelease(p->vertexBuffer);                    p->vertexBuffer           = NULL; }
    if (p->verts)                  { free(p->verts);                                        p->verts                  = NULL; }
    if (p->cmds)                   { free(p->cmds);                                         p->cmds                   = NULL; }
    /* ↑ kind of unholy ↑ */
    
    free(p);
}

static void destroyWindowResources(struct glfwWindow *w) {
    releaseFrame(w);

    destroyPipeline(w->pipeline);
    w->pipeline = NULL;

    wgpuSurfaceUnconfigure(w->surface);
    wgpuSurfaceRelease(w->surface);
    w->surface = NULL;

    glfwDestroyWindow(w->window);
    w->window = NULL;
    
    vanir_log_info("destroyWindowResources: \"%s\" released", w->name);

    // ↓ tear down the shared GPU context when the last window is gone ↓
    if (windowPool.count == 1)
        vanirGPUDestroy();
}
/* ↑ pipeline lifecycle ↑ */

/* ↓ render hook handle ↓ */
void renderHandle(struct hook *instance, lua_State *L) {
    glfwPollEvents();

    /* ↓ deferred window freeing ↓ */
    for (size_t i = 0; i < windowPool.count; ) {
        struct glfwWindow *w = windowPool.windows[i];
       
        if (w->quit) {
            destroyWindowResources(w);
            free(w);
            
            /* ↓ shift remaining windows down to fill gap ↓ */
            for (size_t j = i; j < windowPool.count - 1; ++j)
                windowPool.windows[j] = windowPool.windows[j + 1];
            
            windowPool.windows[windowPool.count - 1] = NULL;
            windowPool.count -= 1;
        } else {
            ++i;
        }
    }

    instance->status = (windowPool.count == 0) ? hook_idle : hook_update;
}
/* ↑ render hook handle ↑ */

struct hook render = {"render", NULL, 0, &render, renderHandle, hook_update};

/* window metafunctions ↓↓↓ window metafunctions */
int isHovering(lua_State *L) {
    struct glfwWindow **w = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
    
    lua_pushboolean(L, (*w)->hovering);
    
    return 1;
}

int isFocused(lua_State *L) {
    struct glfwWindow **w = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
    
    lua_pushboolean(L, (*w)->focused);
    
    return 1;
}

int getTitle(lua_State *L) {
    struct glfwWindow **w = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
    
    lua_pushstring(L, (*w)->name);
    
    return 1;
}

int getID(lua_State *L) {
    struct glfwWindow **w = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
    
    lua_pushinteger(L, (lua_Integer)(uintptr_t)(*w)->window);
    
    return 1;
}

/* ↓ returns nil, nil if mouse is off window ↓ */
int getMouse(lua_State *L) {
    struct glfwWindow **window = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
   
    if (!(*window)->hovering) { 
        lua_pushnil(L);
        lua_pushnil(L);
        
        return 2;
    }
    
    double x, y;
    
    glfwGetCursorPos((*window)->window, &x, &y);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    
    return 2;
}

/* ↓ returns screen coords, not framebuffer pixels; uses fbWidth/fbHeight internally for gpu operations ↓ */
int getSize(lua_State *L) {
    struct glfwWindow **w = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
   
    lua_pushinteger(L, (*w)->width);
    lua_pushinteger(L, (*w)->height);
    
    return 2;
}

int getMonitorIndex(lua_State *L) {
    struct glfwWindow **window = (struct glfwWindow **)luaL_checkudata(L, 1, "window");
    int count;
    
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    GLFWmonitor *mon = glfwGetWindowMonitor((*window)->window);
    
    int index = 0;
    
    for (int i = 0; i < count; ++i)
        if (monitors[i] == mon) { 
            index = i;
            
            break;
        }
    
    lua_pushinteger(L, index);
    
    return 1;
}
/* window metafunctions ↑↑↑ window metafunctions */

/* pipeline / shader ↓↓↓ pipeline / shader */
static const char *VANIR_WGSL =
    "struct Viewport { width: f32, height: f32 }\n"
    "@group(0) @binding(0) var<uniform> viewport: Viewport;\n"
    "\n"
    "struct VertIn  { @location(0) pos : vec3f, @location(1) color : vec4f }\n"
    "struct VertOut { @builtin(position) pos : vec4f, @location(0) color : vec4f }\n"
    "@vertex\n"
    "fn vs_main(v: VertIn) -> VertOut {\n"
    "    var out: VertOut;\n"
    "    let ndcX =  (v.pos.x / viewport.width)  * 2.0 - 1.0;\n"
    "    let ndcY = -(v.pos.y / viewport.height) * 2.0 + 1.0;\n"
    "    out.pos   = vec4f(ndcX, ndcY, v.pos.z, 1.0);\n"
    "    out.color = v.color;\n"
    "    return out;\n"
    "}\n"
    "@fragment\n"
    "fn fs_main(in: VertOut) -> @location(0) vec4f { return in.color; }\n";
/* ↑ converts pixel-space -> NOC using viewport uniform (width / height); used by all draw calls ↑ */

/* ↓ called twice for triangles & lines; builds single WGPURenderPipeline for primitive topology and blend ↓  */
static WGPURenderPipeline buildRenderPipeline(struct glfwWindow *w, WGPUPrimitiveTopology topology, WGPUBindGroupLayout bgl, WGPUBlendFactor blendSrc, WGPUBlendFactor blendDst) {
    WGPUShaderSourceWGSL wgsl_src = {0};
    wgsl_src.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl_src.code.data = VANIR_WGSL;
    wgsl_src.code.length = strlen(VANIR_WGSL);

    WGPUShaderModuleDescriptor shader_desc = {0};
    shader_desc.nextInChain = &wgsl_src.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(gpu.device, &shader_desc);
    /* ↑ compile wgsl shader module ↑ */
    
    if (!shader) { 
        throw("buildRenderPipeline", w->name, "wgpuDeviceCreateShaderModule failed");
        
        return NULL;
    }

    /* ↓ vertex buffer layout; slot 0, stride 7 floats, 2 attributes ↓ */
    WGPUVertexAttribute attrs[2] = {0};
    attrs[0].format = WGPUVertexFormat_Float32x3;
    attrs[0].offset = 0;
    attrs[0].shaderLocation = 0;
    attrs[1].format = WGPUVertexFormat_Float32x4;
    attrs[1].offset = 3*sizeof(float);
    attrs[1].shaderLocation = 1;

    WGPUVertexBufferLayout vbl = {0};
    vbl.arrayStride = 7*sizeof(float);
    vbl.stepMode = WGPUVertexStepMode_Vertex;
    vbl.attributeCount = 2;
    vbl.attributes = attrs;

    /* ↓ alpha blending; output = src * srcFactor + dst * dstFactor ↓ */
    WGPUBlendState blend = {0};
    blend.color.srcFactor = blendSrc;
    blend.color.dstFactor = blendDst;
    blend.color.operation = WGPUBlendOperation_Add;
    blend.alpha.srcFactor = WGPUBlendFactor_One;
    blend.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState color_target = {0};
    color_target.format = w->surfaceFormat;
    color_target.writeMask = WGPUColorWriteMask_All;
    color_target.blend = &blend;

    WGPUFragmentState frag = {0};
    frag.module = shader;
    frag.entryPoint = (WGPUStringView){ .data = "fs_main", .length = 7 };
    frag.targetCount = 1;
    frag.targets = &color_target;

    WGPUPipelineLayoutDescriptor pl_desc = {0};
    pl_desc.bindGroupLayoutCount = 1;
    pl_desc.bindGroupLayouts = &bgl;
    WGPUPipelineLayout pl = wgpuDeviceCreatePipelineLayout(gpu.device, &pl_desc);

    WGPURenderPipelineDescriptor pipe_desc = {0};
    pipe_desc.layout = pl;
    pipe_desc.vertex.module = shader;
    pipe_desc.vertex.entryPoint = (WGPUStringView){ .data = "vs_main", .length = 7 };
    pipe_desc.vertex.bufferCount = 1;
    pipe_desc.vertex.buffers = &vbl;
    pipe_desc.fragment = &frag;
    pipe_desc.primitive.topology = topology;
    pipe_desc.multisample.count = 1;
    pipe_desc.multisample.mask = 0xFFFFFFFF;

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(gpu.device, &pipe_desc);
    
    /* ↓ pipeline layout and shader are referecne-counted; release after ownership transfer ↓ */
    wgpuPipelineLayoutRelease(pl);
    wgpuShaderModuleRelease(shader);
    
    if (!pipeline) 
        throw("buildRenderPipeline", w->name, "wgpuDeviceCreateRenderPipeline failed");
    
    return pipeline;
}

/* ↓ build pipeline bundle; cpu side vertex/command batch buffers grow on demand ↓ */
/* ↓ TODO: note later about bundle ↓*/
struct Pipeline *buildPipelines(struct glfwWindow *w, WGPUBlendFactor blendSrc, WGPUBlendFactor blendDst) {
    struct Pipeline *p = calloc(1, sizeof(struct Pipeline));
    
    if (!p) { 
        throw("buildPipelines", w->name, "calloc failed");
        
        return NULL;
    }

    p->blendSrc = blendSrc;
    p->blendDst = blendDst;

    /* ↓ bind group layout; one uniform buffer -> vertex stage ↓ */
    WGPUBindGroupLayoutEntry bgl_entry = {0};
    bgl_entry.binding = 0;
    bgl_entry.visibility = WGPUShaderStage_Vertex;
    bgl_entry.buffer.type = WGPUBufferBindingType_Uniform;
    bgl_entry.buffer.minBindingSize = 2*sizeof(float);

    WGPUBindGroupLayoutDescriptor bgl_desc = {0};
    bgl_desc.entryCount = 1;
    bgl_desc.entries = &bgl_entry;
    p->uniformBindGroupLayout = wgpuDeviceCreateBindGroupLayout(gpu.device, &bgl_desc);
    
    if (!p->uniformBindGroupLayout) { 
        throw("buildPipelines", w->name, "wgpuDeviceCreateBindGroupLayout failed");
        destroyPipeline(p);
        
        return NULL;
    }

    /* ↓ uniform buffer; 2 floats (width, height). updated each frame ↓ */
    WGPUBufferDescriptor ub_desc = {0};
    ub_desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    ub_desc.size = 2*sizeof(float);
    ub_desc.mappedAtCreation = false;
    p->uniformBuffer = wgpuDeviceCreateBuffer(gpu.device, &ub_desc);
    
    if (!p->uniformBuffer) { 
        throw("buildPipelines", w->name, "wgpuDeviceCreateBuffer (uniform) failed");
        destroyPipeline(p);
        
        return NULL;
    }

    /* ↓ upload initial viewport ↓ */
    float vp[2] = { (float)w->fbWidth, (float)w->fbHeight };
    wgpuQueueWriteBuffer(gpu.queue, p->uniformBuffer, 0, vp, sizeof(vp));

    /* ↓ bind group; wire binding 0 -> uniformBuffer ↓ */
    WGPUBindGroupEntry bg_entry = {0};
    bg_entry.binding = 0;
    bg_entry.buffer = p->uniformBuffer;
    bg_entry.size = 2*sizeof(float);

    WGPUBindGroupDescriptor bg_desc = {0};
    bg_desc.layout = p->uniformBindGroupLayout;
    bg_desc.entryCount = 1;
    bg_desc.entries = &bg_entry;
    p->uniformBindGroup = wgpuDeviceCreateBindGroup(gpu.device, &bg_desc);
    
    if (!p->uniformBindGroup) { 
        throw("buildPipelines", w->name, "wgpuDeviceCreateBindGroup failed");
        destroyPipeline(p);
        
        return NULL;
    }

    p->pipeline = buildRenderPipeline(w, WGPUPrimitiveTopology_TriangleList, p->uniformBindGroupLayout, blendSrc, blendDst);
    p->pipelineLine = buildRenderPipeline(w, WGPUPrimitiveTopology_LineList, p->uniformBindGroupLayout, blendSrc, blendDst);
    
    if (!p->pipeline || !p->pipelineLine) { 
        destroyPipeline(p);
        
        return NULL;
    }

    vanir_log_info("buildPipelines: ready for \"%s\"", w->name);
    
    return p;
}
/* pipeline / shader ↑↑↑ pipeline / shader */

/* ↓ window creation; glfw -> webgpu surface. on first call, init shared gpu context (instance -> adapter -> device -> queue) ↓ */
static void newWindow(struct glfwWindow *window) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window->window = glfwCreateWindow(window->width, window->height, window->name, NULL, NULL);
    
    if (!window->window) { 
        throw("Window", window->name, "glfwCreateWindow failed");
    
        return;
    }

    /* ↓ glfw event callback reg ↓ */
    glfwSetWindowPos(window->window, window->x, window->y);
    glfwSetFramebufferSizeCallback(window->window, cbResize);
    glfwSetCursorEnterCallback(window->window, cbCursorEnter);
    glfwSetWindowFocusCallback(window->window, cbFocus);
    glfwSetWindowCloseCallback(window->window, cbClose);
    glfwGetWindowSize(window->window, &window->width, &window->height);
    glfwGetFramebufferSize(window->window, &window->fbWidth, &window->fbHeight);
    /* ↑ glfw event callback reg ↑ */

    /* ↓ lazy gpu init: create the instance on the first window only ↓ */
    if (!gpu.instance) {
        WGPUInstanceDescriptor inst_desc = {0};
        gpu.instance = wgpuCreateInstance(&inst_desc);
        
        if (!gpu.instance) {
            throw("GPU", "init", "wgpuCreateInstance failed");
            
            glfwDestroyWindow(window->window);
            
            window->window = NULL;
           
            return;
        }
    }

    /* ↓ create the platform-specific surface from glfw3webgpu.h ↓ */
    window->surface = glfwGetWGPUSurface(gpu.instance, window->window);
    
    if (!window->surface) {
        throw("Window", window->name, "glfwGetWGPUSurface failed");
        
        glfwDestroyWindow(window->window);
        
        window->window = NULL;
        
        return;
    }

    /* ↓ request adapter and device on the first window only ↓ */
    if (!gpu.adapter) {
        WGPURequestAdapterOptions adapter_opts = {0};
        adapter_opts.compatibleSurface = window->surface;
        WGPURequestAdapterCallbackInfo adapter_cb = {0};
        adapter_cb.callback = on_adapter;
        adapter_cb.userdata1 = &gpu.adapter;
        
        wgpuInstanceRequestAdapter(gpu.instance, &adapter_opts, adapter_cb);

        if (!gpu.adapter) {
            throw("GPU", "init", "wgpuInstanceRequestAdapter failed");
            
            wgpuSurfaceRelease(window->surface);
            window->surface = NULL;

            glfwDestroyWindow(window->window);
            window->window = NULL;

            wgpuInstanceRelease(gpu.instance);
            gpu.instance = NULL;
            
            return;
        }

        WGPUDeviceDescriptor device_desc = {0};
        WGPURequestDeviceCallbackInfo device_cb = {0};
        device_cb.callback = on_device;
        device_cb.userdata1 = &gpu.device;
        
        wgpuAdapterRequestDevice(gpu.adapter, &device_desc, device_cb);

        if (!gpu.device) {
            throw("GPU", "init", "wgpuAdapterRequestDevice failed");
            
            wgpuSurfaceRelease(window->surface);window->surface = NULL;
            glfwDestroyWindow(window->window);window->window = NULL;
            wgpuAdapterRelease(gpu.adapter);gpu.adapter = NULL;
            wgpuInstanceRelease(gpu.instance);gpu.instance = NULL;
            
            return;
        }

        gpu.queue = wgpuDeviceGetQueue(gpu.device);
        
        vanir_log_info("vanirGPUInit: shared device + queue ready");
    }

    /* ↓ query surface for supported formats/modes ↓ */
    WGPUSurfaceCapabilities caps = {0};
    wgpuSurfaceGetCapabilities(window->surface, gpu.adapter, &caps);
    window->surfaceFormat = caps.formats[0];
    
    /* prefer mailbox (low-latency) > immediate (no vsync) > fifo (vsync) ↓ */
    WGPUPresentMode presentMode = WGPUPresentMode_Fifo;
    for (size_t i = 0; i < caps.presentModeCount; ++i) {
        if (caps.presentModes[i] == WGPUPresentMode_Mailbox) {
            presentMode = WGPUPresentMode_Mailbox;
            
            break;
        }
        
        if (caps.presentModes[i] == WGPUPresentMode_Immediate)
            presentMode = WGPUPresentMode_Immediate;
    }
    
    window->presentMode = presentMode;

    /* ↓ normal webgpu surface config ↓ */
    WGPUSurfaceConfiguration surf_cfg = {0};
    surf_cfg.device = gpu.device;
    surf_cfg.format = window->surfaceFormat;
    surf_cfg.usage = WGPUTextureUsage_RenderAttachment;
    surf_cfg.width = (uint32_t)window->fbWidth;
    surf_cfg.height = (uint32_t)window->fbHeight;
    surf_cfg.presentMode = presentMode;
    /* ↑ normal webgpu surface config ↑ */
    
    wgpuSurfaceConfigure(window->surface, &surf_cfg);

    struct glfwWindow **temp = realloc(windowPool.windows, (windowPool.count + 1) * sizeof(struct glfwWindow *));
    
    if (!temp) {
        throw("Window", window->name, "Memory allocation error");
        wgpuSurfaceRelease(window->surface);
        glfwDestroyWindow(window->window);
        
        window->surface = NULL;
        window->window = NULL;
       
        return;
    }

    windowPool.windows = temp;
    windowPool.windows[windowPool.count++] = window;

    /* ↓ triangles + line ↓ */
    window->pipeline = buildPipelines(window, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha);
}

static const luaL_Reg windowMethods[] = {
    {"selectRender", selectRender},
    {"stopRender", stopRender},
    {"update", update},
    {"isHovering", isHovering},
    {"isFocused", isFocused},
    {"getTitle", getTitle},
    {"getID", getID},
    {"getMouse", getMouse},
    {"getSize", getSize},
    {"getMonitorIndex", getMonitorIndex},
    /* ↑ window metafunctions ↑ */

    {NULL, NULL}
};

/* ↓ lua interface for internal newWindow ↓ */
int createWindow(lua_State *L) {
    int x = luaL_optinteger(L, 1, 300);
    int y = luaL_optinteger(L, 2, 300);
    int width = luaL_optinteger(L, 3, 300);
    int height = luaL_optinteger(L, 4, 200);
    const char *name = luaL_optstring(L, 5, "Vanir window");

    struct glfwWindow *window = calloc(1, sizeof(struct glfwWindow));
    
    if (!window) { 
        throw("Window", name, "Memory allocation error");
        
        return 0;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->name = name;
    window->ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* ↓ push userdata before calling so window is reachable by gc ↓ */
    struct glfwWindow **udata = (struct glfwWindow **)lua_newuserdata(L, sizeof(struct glfwWindow *));
    *udata = window;
    
    addMethods(L, "window", windowMethods, NULL);

    /* ↓ main sauce ↓ */
    newWindow(window);

    if (!window->window) { 
        free(window);
        
        return 0;
    }

    window->focused = glfwGetWindowAttrib(window->window, GLFW_FOCUSED) != 0;
    window->hovering = glfwGetWindowAttrib(window->window, GLFW_HOVERED) != 0;

    /* ↓ allocate callbacks for event hooks ↓ */
    onHoverChange.callback = createCallback(sizeof(bool), lua_bool);
    onFocusChange.callback = createCallback(sizeof(bool), lua_bool);
    onEvent.callback = createCallback(sizeof(int),  integer);
    onOpen.status = hook_awaiting;

    return 1;
}

const luaL_Reg luaWindows[] = {
    {"createWindow", createWindow},
    
    {NULL, NULL}
};

int windowsInit(lua_State *L) {
    if (!glfwInit()) { 
        throw("Init", "GLFW", "glfwInit failed");
    
        return 1;
    }

    luaL_newlib(L, luaWindows);

    registerHook(render);
    registerHook(onHoverChange);
    registerHook(onFocusChange);
    registerHook(onResize);
    registerHook(onEvent);
    registerHook(onClose);
    registerHook(onOpen);

    return 1;
}