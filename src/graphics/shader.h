#ifndef SHADER_H
#define SHADER_H

#include <webgpu/webgpu.h>
#include <stdbool.h>
#include <stdint.h>

#include "../vanir.h"

/* ↓ forward declarations to avoid circular includes ↓ */
struct glfwWindow;
struct Texture;

/* ↓ a compiled custom WGSL shader with its pipeline ↓ */
struct Shader {
    const char *name;

    WGPURenderPipeline pipeline;
    WGPURenderPipeline pipelineLine;

    /* ↓ vertex attribute layout expected by the shader; defaults match the built-in layout ↓ */
    uint32_t vertexStride;        // bytes per vertex
    bool hasUV;                   // ↓ true when the shader uses UV coords (location 2) ↓
};

/* ↓ global shader pool ↓ */
struct ShaderPool {
    struct Shader **shaders;
    int count;
};

extern struct ShaderPool shaderPool;

/* ↓ the currently active custom shader; NULL means the built-in pipeline is used ↓ */
/* ↓ respected by beginPass in render.c whenever a new render pass opens ↓ */
extern struct Shader *activeShader;

/* ↓ find a compiled shader by name; NULL if not found ↓ */
struct Shader *findShader(const char *name);

/* ↓ compile a custom WGSL shader and register it; returns false on error ↓ */
bool shaderCompile(const char *name, const char *wgsl_src, bool hasUV);

/* ↓ draw a textured quad immediately into the current render pass ↓ */
/* ↓ flushes any pending geometry first so draw order is preserved ↓ */
void drawTexturedQuadImmediate(struct glfwWindow *w,
                                struct Texture *tex,
                                float dx, float dy, float dw, float dh,
                                float u0, float v0, float u1, float v1);

/* ↓ flush pending batched geometry before switching to textured pipeline ↓ */
void flushBatchesTextured(struct glfwWindow *w);

/* ↓ Lua bindings ↓ */
int shaderInit(lua_State *L);

/* ↓ destroy all shaders; called on quit ↓ */
void destroyAllShaders(void);

#endif
