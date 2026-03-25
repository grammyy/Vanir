#ifndef TEXTURES_H
#define TEXTURES_H

#include <webgpu/webgpu.h>
#include <stdint.h>
#include <stdbool.h>

#include "../vanir.h"

/* ↓ forward declaration to avoid circular includes ↓ */
struct glfwWindow;

/* ↓ a loaded GPU texture with its view, sampler, and bind group for sampling ↓ */
struct Texture {
    const char       *name;       // ↓ user-given label (defaults to path) ↓
    const char       *path;       // ↓ original file path (NULL for loadFromPixels) ↓

    uint8_t *pixels;
    WGPUTexture       texture;
    WGPUTextureView   view;
    WGPUSampler       sampler;

    /* ↓ bind group for use with the textured pipeline (group 1: binding 0 = tex, 1 = sampler) ↓ */
    WGPUBindGroup           bindGroup;
    WGPUBindGroupLayout     bindGroupLayout;

    uint32_t width, height;
    int      channels;            // ↓ original channel count from stb_image (0 if loadFromPixels) ↓
    size_t   fileSize;            // ↓ file size in bytes at load time (0 if loadFromPixels) ↓
    WGPUTextureFormat format;
};

/* ↓ global pool ↓ */
struct TexturePool {
    struct Texture **textures;
    int count;
};

extern struct TexturePool texturePool;

/* ↓ find by name, NULL if not found ↓ */
struct Texture *findTexture(const char *name);

/* ↓ upload raw RGBA8 pixel data to the GPU ↓ */
struct Texture *textureUpload(const char *name, const uint8_t *pixels, uint32_t w, uint32_t h);

/* ↓ free all GPU resources for a texture and remove it from the pool ↓ */
void textureRelease(struct Texture *tex);

/* ↓ load a new image from path into an existing texture; replaces GPU resources in-place ↓ */
/* ↓ also updates tex->path, tex->channels, tex->fileSize; tex pointer stays valid ↓ */
bool textureSetImage(struct Texture *tex, const char *path);

/* ↓ read a pixel from cpu-side data; returns false if out of range or no pixel data ↓ */
bool textureGetColor(struct Texture *tex, uint32_t x, uint32_t y, float *r, float *g, float *b, float *a);

/* ↓ Lua bindings ↓ */
int texturesInit(lua_State *L);

/* ↓ draw a texture (or portion of one) as a quad on the current window ↓ */
/* ↓ called internally by setRenderTargetTexture; also exposed to Lua ↓ */
int drawTexturedRect(lua_State *L);

/* ↓ destroy all textures; called on quit ↓ */
void destroyAllTextures(void);

#endif
