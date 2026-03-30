#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <stdlib.h>
#include <string.h>

#include "../vanir.h"
#include "../modules/windows.h"
#include "../modules/render.h"
#include "textures.h"
#include "shader.h"

extern struct VanirGPU gpu;
extern struct windowPool windowPool;

/* ↓ from graphics/draw.c; declared here to avoid including graphics/render.h which has struct conflicts ↓ */
extern void flushBatches(struct glfwWindow *w);

struct ShaderPool shaderPool = {NULL, 0};

/* ↓ the currently active custom shader; NULL means the built-in pipeline is used ↓ */
struct Shader *activeShader = NULL;

struct Shader *findShader(const char *name) {
    for (int i = 0; i < shaderPool.count; ++i)
        if (strcmp(shaderPool.shaders[i]->name, name) == 0)
            return shaderPool.shaders[i];

    return NULL;
}

/* ↓ compile a WGSL source string into a RenderPipeline for triangle and line topology ↓ */
/* ↓ the shader must follow the same vertex layout as the built-in pipeline ↓ */
/* ↓ (location0=pos:vec3f, location1=color:vec4f) and expose vs_main/fs_main ↓ */
bool shaderCompile(const char *name, const char *wgsl_src, bool hasUV) {
    if (!gpu.device) {
        throw("shader.compile", name, "no GPU device — call createWindow first");

        return false;
    }

    if (windowPool.count == 0) {
        throw("shader.compile", name, "no windows open; pipeline needs a surface format");

        return false;
    }

    if (findShader(name)) {
        vanir_log_info("shader.compile: \"%s\" already compiled, replacing", name);

        /* ↓ release old shader with this name ↓ */
        for (int i = 0; i < shaderPool.count; ++i) {
            if (strcmp(shaderPool.shaders[i]->name, name) == 0) {
                struct Shader *old = shaderPool.shaders[i];

                if (old->pipeline)
                    wgpuRenderPipelineRelease(old->pipeline);
                
                    if (old->pipelineLine)
                    wgpuRenderPipelineRelease(old->pipelineLine);

                free(old);

                shaderPool.shaders[i] = shaderPool.shaders[--shaderPool.count];

                break;
            }
        }
    }

    struct glfwWindow *w = windowPool.windows[0];
    struct Pipeline *p = w->pipeline;

    WGPUShaderSourceWGSL src = {0};
    src.chain.sType = WGPUSType_ShaderSourceWGSL;
    src.code.data = wgsl_src;
    src.code.length = strlen(wgsl_src);

    WGPUShaderModuleDescriptor desc = {0};
    desc.nextInChain = &src.chain;
    WGPUShaderModule mod = wgpuDeviceCreateShaderModule(gpu.device, &desc);

    if (!mod) {
        throw("shader.compile", name, "wgpuDeviceCreateShaderModule failed — check WGSL syntax");

        return false;
    }

    /* ↓ vertex layout mirrors the built-in pipeline ↓ */
    WGPUVertexAttribute attrs[2] = {0};
    attrs[0].format = WGPUVertexFormat_Float32x3;
    attrs[0].offset = 0;
    attrs[0].shaderLocation = 0;
    attrs[1].format = WGPUVertexFormat_Float32x4;
    attrs[1].offset = 3 * sizeof(float);
    attrs[1].shaderLocation = 1;

    WGPUVertexBufferLayout vbl = {0};
    vbl.arrayStride = 7 * sizeof(float);
    vbl.stepMode = WGPUVertexStepMode_Vertex;
    vbl.attributeCount = 2;
    vbl.attributes = attrs;

    WGPUBlendState blend = {0};
    blend.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend.color.operation = WGPUBlendOperation_Add;
    blend.alpha.srcFactor = WGPUBlendFactor_One;
    blend.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState color_target = {0};
    color_target.format = w->surfaceFormat;
    color_target.writeMask = WGPUColorWriteMask_All;
    color_target.blend = &blend;

    WGPUFragmentState frag = {0};
    frag.module = mod;
    frag.entryPoint = (WGPUStringView){ .data = "fs_main", .length = 7 };
    frag.targetCount = 1;
    frag.targets = &color_target;

    /* ↓ reuse the window's existing uniform bind group layout ↓ */
    WGPUPipelineLayoutDescriptor pl_desc = {0};
    pl_desc.bindGroupLayoutCount = 1;
    pl_desc.bindGroupLayouts = &p->uniformBindGroupLayout;
    WGPUPipelineLayout pl = wgpuDeviceCreatePipelineLayout(gpu.device, &pl_desc);

    WGPURenderPipelineDescriptor tri_desc = {0};
    tri_desc.layout = pl;
    tri_desc.vertex.module = mod;
    tri_desc.vertex.entryPoint = (WGPUStringView){ .data = "vs_main", .length = 7 };
    tri_desc.vertex.bufferCount = 1;
    tri_desc.vertex.buffers = &vbl;
    tri_desc.fragment = &frag;
    tri_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    tri_desc.multisample.count = 1;
    tri_desc.multisample.mask = 0xFFFFFFFF;

    WGPURenderPipeline triPipeline = wgpuDeviceCreateRenderPipeline(gpu.device, &tri_desc);

    WGPURenderPipelineDescriptor line_desc = tri_desc;
    line_desc.primitive.topology = WGPUPrimitiveTopology_LineList;
    WGPURenderPipeline linePipeline = wgpuDeviceCreateRenderPipeline(gpu.device, &line_desc);

    wgpuPipelineLayoutRelease(pl);
    wgpuShaderModuleRelease(mod);

    if (!triPipeline || !linePipeline) {
        throw("shader.compile", name, "wgpuDeviceCreateRenderPipeline failed");

        if (triPipeline)
            wgpuRenderPipelineRelease(triPipeline);
        
        if (linePipeline)
            wgpuRenderPipelineRelease(linePipeline);

        return false;
    }

    struct Shader *shader = calloc(1, sizeof(struct Shader));

    if (!shader) {
        throw("shader.compile", name, "calloc failed");
        wgpuRenderPipelineRelease(triPipeline);
        wgpuRenderPipelineRelease(linePipeline);

        return false;
    }

    shader->name = name;
    shader->pipeline = triPipeline;
    shader->pipelineLine = linePipeline;
    shader->vertexStride = 7 * sizeof(float);
    shader->hasUV = hasUV;

    /* ↓ grow pool ↓ */
    struct Shader **temp = realloc(shaderPool.shaders, (shaderPool.count + 1) * sizeof(struct Shader *));

    if (!temp) {
        throw("shader.compile", name, "realloc failed");
        wgpuRenderPipelineRelease(triPipeline);
        wgpuRenderPipelineRelease(linePipeline);
        free(shader);

        return false;
    }

    shaderPool.shaders = temp;
    shaderPool.shaders[shaderPool.count++] = shader;

    vanir_log_info("shader.compile: \"%s\" compiled OK (tri=%p line=%p)", name, (void*)triPipeline, (void*)linePipeline);

    return true;
}

/* ↓ flush pending geometry batches before switching pipelines ↓ */
void flushBatchesTextured(struct glfwWindow *w) {
    flushBatches(w);
}

/* ↓ draw a textured quad directly (non-batched) into the current render pass ↓ */
/* ↓ flushes any pending geometry first so draw order is preserved ↓ */
void drawTexturedQuadImmediate(struct glfwWindow *w, struct Texture *tex, float dx, float dy, float dw, float dh, float u0, float v0, float u1, float v1) {
    struct Pipeline *p = w->pipeline;

    if (!p || !p->pipelineTextured) {
        throw("drawTexturedRect", tex->name, "textured pipeline not available");

        return;
    }

    if (!w->frame.passEncoder) {
        throw("drawTexturedRect", tex->name, "no active render pass");

        return;
    }

    vanir_log_info("drawTexturedQuadImmediate: \"%s\" dst=(%.0f,%.0f,%.0f,%.0f) uv=(%.2f,%.2f,%.2f,%.2f) pipeline=%p", tex->name, dx, dy, dw, dh, u0, v0, u1, v1, (void*)p->pipelineTextured);

    /* ↓ flush pending geometry batches first ↓ */
    if (p->cmdCount > 0)
        flushBatches(w);

    /* ↓ 6 vertices (2 triangles), 6 floats each: x y z w  u v ↓ */
    float verts[6 * TEXTURED_VERTEX_STRIDE] = {
        dx,      dy,      0, 1,   u0, v0,
        dx + dw, dy,      0, 1,   u1, v0,
        dx + dw, dy + dh, 0, 1,   u1, v1,
        dx,      dy,      0, 1,   u0, v0,
        dx + dw, dy + dh, 0, 1,   u1, v1,
        dx,      dy + dh, 0, 1,   u0, v1,
    };

    uint32_t size = sizeof(verts);

    /* ↓ create or reuse a small scratch vertex buffer ↓ */
    WGPUBufferDescriptor buf_desc = {0};
    buf_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    buf_desc.size = size;
    buf_desc.mappedAtCreation = false;
    WGPUBuffer vbuf = wgpuDeviceCreateBuffer(gpu.device, &buf_desc);

    if (!vbuf) {
        throw("drawTexturedRect", tex->name, "wgpuDeviceCreateBuffer failed");

        return;
    }

    wgpuQueueWriteBuffer(gpu.queue, vbuf, 0, verts, size);

    /* ↓ switch to textured pipeline and set bind groups ↓ */
    wgpuRenderPassEncoderSetPipeline(w->frame.passEncoder, p->pipelineTextured);

    if (p->uniformBindGroup)
        wgpuRenderPassEncoderSetBindGroup(w->frame.passEncoder, 0, p->uniformBindGroup, 0, NULL);

    /* ↓ bind the texture's group at slot 1 ↓ */
    wgpuRenderPassEncoderSetBindGroup(w->frame.passEncoder, 1, tex->bindGroup, 0, NULL);

    wgpuRenderPassEncoderSetVertexBuffer(w->frame.passEncoder, 0, vbuf, 0, size);
    wgpuRenderPassEncoderDraw(w->frame.passEncoder, 6, 1, 0, 0);

    vanir_log_info("drawTexturedQuadImmediate: draw call submitted for \"%s\"", tex->name);

    wgpuBufferRelease(vbuf);

    /* ↓ restore the solid-color pipeline so subsequent draw calls work correctly ↓ */
    if (p->pipeline) {
        wgpuRenderPassEncoderSetPipeline(w->frame.passEncoder, p->pipeline);

        if (p->uniformBindGroup)
            wgpuRenderPassEncoderSetBindGroup(w->frame.passEncoder, 0, p->uniformBindGroup, 0, NULL);
    }
}

void destroyAllShaders(void) {
    for (int i = 0; i < shaderPool.count; ++i) {
        struct Shader *s = shaderPool.shaders[i];

        if (s->pipeline)     { wgpuRenderPipelineRelease(s->pipeline);     s->pipeline     = NULL; }
        if (s->pipelineLine) { wgpuRenderPipelineRelease(s->pipelineLine); s->pipelineLine = NULL; }

        free(s);
    }

    free(shaderPool.shaders);

    shaderPool.shaders = NULL;
    shaderPool.count   = 0;
}

/* ↓ Lua bindings ↓ */

/* ↓ shader.compile(name, wgsl_source) ↓ */
static int luaShaderCompile(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    const char *src  = luaL_checkstring(L, 2);

    lua_pushboolean(L, shaderCompile(name, src, false) ? 1 : 0);

    return 1;
}

/* ↓ shader.setActive(name) — draw calls use this pipeline instead of the built-in one ↓ */
/* ↓ shader.setActive(nil) — revert to the built-in pipeline ↓ */
static int luaShaderSetActive(lua_State *L) {
    if (lua_isnil(L, 1) || lua_isnone(L, 1)) {
        activeShader = NULL;

        vanir_log_info("shader.setActive: reverted to built-in pipeline");

        /* ↓ restore the built-in pipeline on all windows ↓ */
        for (int i = 0; i < windowPool.count; ++i) {
            struct glfwWindow *w = windowPool.windows[i];

            if (w->pipeline && w->pipeline->pipeline && w->frame.passEncoder) {
                wgpuRenderPassEncoderSetPipeline(w->frame.passEncoder, w->pipeline->pipeline);

                if (w->pipeline->uniformBindGroup)
                    wgpuRenderPassEncoderSetBindGroup(w->frame.passEncoder, 0, w->pipeline->uniformBindGroup, 0, NULL);
            }
        }

        return 0;
    }

    const char *name = luaL_checkstring(L, 1);
    struct Shader *s = findShader(name);

    if (!s) {
        throw("shader.setActive", name, "shader not found — compile it first");

        return 0;
    }

    activeShader = s;

    vanir_log_info("shader.setActive: \"%s\" (tri=%p line=%p)", name, (void*)s->pipeline, (void*)s->pipelineLine);

    /* ↓ switch the render pass on any active windows immediately ↓ */
    for (int i = 0; i < windowPool.count; ++i) {
        struct glfwWindow *w = windowPool.windows[i];

        if (!w->frame.passEncoder) 
            continue;

        struct Pipeline *p = w->pipeline;

        /* ↓ flush pending geometry before switching pipeline ↓ */
        if (p && p->cmdCount > 0)
            flushBatches(w);

        wgpuRenderPassEncoderSetPipeline(w->frame.passEncoder, s->pipeline);

        if (p && p->uniformBindGroup)
            wgpuRenderPassEncoderSetBindGroup(w->frame.passEncoder, 0, p->uniformBindGroup, 0, NULL);
    }

    return 0;
}

/* ↓ shader.release(name) ↓ */
static int luaShaderRelease(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);

    for (int i = 0; i < shaderPool.count; ++i) {
        struct Shader *s = shaderPool.shaders[i];

        if (strcmp(s->name, name) != 0)
            continue;

        if (activeShader == s)
            activeShader = NULL;

        vanir_log_info("shader.release: \"%s\"", name);

        if (s->pipeline) 
            wgpuRenderPipelineRelease(s->pipeline);
        
            if (s->pipelineLine) 
            wgpuRenderPipelineRelease(s->pipelineLine);

        free(s);

        shaderPool.shaders[i] = shaderPool.shaders[--shaderPool.count];

        return 0;
    }

    throw("shader.release", name, "shader not found");

    return 0;
}

static int luaShaderExists(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);

    lua_pushboolean(L, findShader(name) != NULL ? 1 : 0);

    return 1;
}

/* ↓ shader.getActive() — returns the name of the active custom shader, or nil ↓ */
static int luaShaderGetActive(lua_State *L) {
    if (activeShader)
        lua_pushstring(L, activeShader->name);
    else
        lua_pushnil(L);

    return 1;
}

/* ↓ shader.list() — returns a table of all compiled shader names ↓ */
static int luaShaderList(lua_State *L) {
    lua_newtable(L);

    for (int i = 0; i < shaderPool.count; ++i) {
        lua_pushstring(L, shaderPool.shaders[i]->name);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

/* ↓ uniform injection ↓ */
/* ↓ shader.setUniform(name, key, value) — store a pending uniform for a named shader ↓ */
/* ↓ values are stored as Lua numbers; send them to the GPU via shader.sendUniforms() ↓ */
static int luaShaderSetUniform(lua_State *L) {
    const char    *shaderName = luaL_checkstring(L, 1);
    const char    *key        = luaL_checkstring(L, 2);
    lua_Number     val        = luaL_checknumber(L, 3);

    struct Shader *s = findShader(shaderName);

    if (!s) {
        throw("shader.setUniform", shaderName, "shader not found — compile it first");
        return 0;
    }

    /* ↓ uniforms table lives in the Lua registry: registry[shaderName] = { key = value, … } ↓ */
    lua_pushstring(L, shaderName);
    lua_gettable(L, LUA_REGISTRYINDEX);

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushstring(L, shaderName);
        lua_pushvalue(L, -2);
        lua_settable(L, LUA_REGISTRYINDEX);
    }

    lua_pushstring(L, key);
    lua_pushnumber(L, val);
    lua_rawset(L, -3);

    lua_pop(L, 1);

    return 0;
}

/* ↓ shader.getUniform(name, key) — read back a stored uniform value, or nil ↓ */
static int luaShaderGetUniform(lua_State *L) {
    const char *shaderName = luaL_checkstring(L, 1);
    const char *key        = luaL_checkstring(L, 2);

    lua_pushstring(L, shaderName);
    lua_gettable(L, LUA_REGISTRYINDEX);

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, key);
    lua_rawget(L, -2);

    /* ↓ result is now on top; remove the table beneath it ↓ */
    lua_remove(L, -2);

    return 1;
}

/* ↓ shader.getUniforms(name) — return the full uniform table for a shader (or nil) ↓ */
static int luaShaderGetUniforms(lua_State *L) {
    const char *shaderName = luaL_checkstring(L, 1);

    lua_pushstring(L, shaderName);
    lua_gettable(L, LUA_REGISTRYINDEX);

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_pushnil(L);
    }

    return 1;
}

static const luaL_Reg luaShader[] = {
    {"compile",      luaShaderCompile},
    {"setActive",    luaShaderSetActive},
    {"getActive",    luaShaderGetActive},
    {"release",      luaShaderRelease},
    {"exists",       luaShaderExists},
    {"list",         luaShaderList},
    {"setUniform",   luaShaderSetUniform},
    {"getUniform",   luaShaderGetUniform},
    {"getUniforms",  luaShaderGetUniforms},
    
    {NULL, NULL}
};

int shaderInit(lua_State *L) {
    luaL_newlib(L, luaShader);

    return 1;
}