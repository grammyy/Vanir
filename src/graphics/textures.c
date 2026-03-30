#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* stb_image ↓↓↓ stb_image */
#define STBI_NO_THREAD_LOCALS
#define STBI_NO_SIMD
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
/* stb_image ↑↑↑ stb_image */

#include "../vanir.h"
#include "../types/common.h"
#include "../modules/windows.h"
#include "../modules/render.h"
#include "textures.h"
#include "shader.h"

extern struct VanirGPU gpu;
extern struct windowPool windowPool;
extern struct Texture *activeTexture;

struct TexturePool texturePool = {NULL, 0};

struct Texture *findTexture(const char *name) {
    for (int i = 0; i < texturePool.count; ++i)
        if (strcmp(texturePool.textures[i]->name, name) == 0)
            return texturePool.textures[i];

    return NULL;
}

static void pushColorValue(lua_State *L, float r, float g, float b, float a) {
    lua_getglobal(L, "Color");

    if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, r);
        lua_pushnumber(L, g);
        lua_pushnumber(L, b);
        lua_pushnumber(L, a);
        lua_call(L, 4, 1);

        return;
    }

    lua_pop(L, 1);

    lua_newtable(L);
    setFieldNumber(L, "r", r);
    setFieldNumber(L, "g", g);
    setFieldNumber(L, "b", b);
    setFieldNumber(L, "a", a);
}

/* ↓ build a sampler + bind group for sampling this texture ↓ */
static bool buildTextureBindGroup(struct Texture *tex) {
    WGPUBindGroupLayoutEntry entries[2] = {0};

    entries[0].binding    = 0;
    entries[0].visibility = WGPUShaderStage_Fragment;
    entries[0].texture.sampleType    = WGPUTextureSampleType_Float;
    entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

    entries[1].binding    = 1;
    entries[1].visibility = WGPUShaderStage_Fragment;
    entries[1].sampler.type = WGPUSamplerBindingType_Filtering;

    WGPUBindGroupLayoutDescriptor bgl_desc = {0};
    bgl_desc.entryCount = 2;
    bgl_desc.entries    = entries;
    tex->bindGroupLayout = wgpuDeviceCreateBindGroupLayout(gpu.device, &bgl_desc);

    if (!tex->bindGroupLayout) {
        throw("Texture", tex->name, "wgpuDeviceCreateBindGroupLayout failed");

        return false;
    }

    WGPUSamplerDescriptor samp_desc = {0};
    samp_desc.addressModeU  = WGPUAddressMode_ClampToEdge;
    samp_desc.addressModeV  = WGPUAddressMode_ClampToEdge;
    samp_desc.magFilter     = WGPUFilterMode_Linear;
    samp_desc.minFilter     = WGPUFilterMode_Linear;
    samp_desc.mipmapFilter  = WGPUMipmapFilterMode_Nearest;
    samp_desc.maxAnisotropy = 1;
    tex->sampler = wgpuDeviceCreateSampler(gpu.device, &samp_desc);

    if (!tex->sampler) {
        throw("Texture", tex->name, "wgpuDeviceCreateSampler failed");

        return false;
    }

    WGPUBindGroupEntry bg_entries[2] = {0};
    bg_entries[0].binding     = 0;
    bg_entries[0].textureView = tex->view;
    bg_entries[1].binding     = 1;
    bg_entries[1].sampler     = tex->sampler;

    WGPUBindGroupDescriptor bg_desc = {0};
    bg_desc.layout     = tex->bindGroupLayout;
    bg_desc.entryCount = 2;
    bg_desc.entries    = bg_entries;
    tex->bindGroup = wgpuDeviceCreateBindGroup(gpu.device, &bg_desc);

    if (!tex->bindGroup) {
        throw("Texture", tex->name, "wgpuDeviceCreateBindGroup failed");

        return false;
    }

    return true;
}

/* ↓ upload raw RGBA8 pixels to the gpu; returns the new Texture or NULL on error ↓ */
struct Texture *textureUpload(const char *name, const uint8_t *pixels, uint32_t w, uint32_t h) {
    if (!gpu.device) {
        throw("Texture", name, "no GPU device — call createWindow first");

        return NULL;
    }

    struct Texture *tex = calloc(1, sizeof(struct Texture));

    if (!tex) {
        throw("Texture", name, "calloc failed");

        return NULL;
    }

    tex->name   = name;
    tex->width  = w;
    tex->height = h;
    tex->format = (windowPool.count > 0)
        ? windowPool.windows[0]->surfaceFormat
        : WGPUTextureFormat_BGRA8Unorm;

    tex->pixels = malloc((size_t)(4 * w * h));

    if (!tex->pixels) {
        throw("Texture", name, "malloc failed");
        free(tex);

        return NULL;
    }

    memcpy(tex->pixels, pixels, (size_t)(4 * w * h));

    vanir_log_info("textureUpload: \"%s\" %ux%u uploading pixels", name, w, h);

    WGPUTextureDescriptor tex_desc = {0};
    tex_desc.usage     = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    tex_desc.dimension = WGPUTextureDimension_2D;
    tex_desc.size      = (WGPUExtent3D){ w, h, 1 };
    tex_desc.format    = tex->format;
    tex_desc.mipLevelCount = 1;
    tex_desc.sampleCount   = 1;
    tex->texture = wgpuDeviceCreateTexture(gpu.device, &tex_desc);

    if (!tex->texture) {
        throw("Texture", name, "wgpuDeviceCreateTexture failed");
        free(tex->pixels);
        free(tex);

        return NULL;
    }

    tex->view = wgpuTextureCreateView(tex->texture, NULL);

    if (!tex->view) {
        throw("Texture", name, "wgpuTextureCreateView failed");
        wgpuTextureRelease(tex->texture);
        free(tex->pixels);
        free(tex);

        return NULL;
    }

    /* ↓ upload pixel data ↓ */
    WGPUTexelCopyTextureInfo dst_info = {0};
    dst_info.texture  = tex->texture;
    dst_info.mipLevel = 0;
    dst_info.origin   = (WGPUOrigin3D){ 0, 0, 0 };
    dst_info.aspect   = WGPUTextureAspect_All;

    WGPUTexelCopyBufferLayout layout = {0};
    layout.offset       = 0;
    layout.bytesPerRow  = 4 * w;
    layout.rowsPerImage = h;

    WGPUExtent3D extent = { w, h, 1 };

    wgpuQueueWriteTexture(gpu.queue, &dst_info, pixels, (size_t)(4 * w * h), &layout, &extent);

    if (!buildTextureBindGroup(tex)) {
        wgpuTextureViewRelease(tex->view);
        wgpuTextureRelease(tex->texture);
        free(tex->pixels);
        free(tex);

        return NULL;
    }

    /* ↓ grow pool ↓ */
    struct Texture **temp = realloc(texturePool.textures, (texturePool.count + 1) * sizeof(struct Texture *));

    if (!temp) {
        throw("Texture", name, "realloc failed");
        wgpuBindGroupRelease(tex->bindGroup);
        wgpuBindGroupLayoutRelease(tex->bindGroupLayout);
        wgpuSamplerRelease(tex->sampler);
        wgpuTextureViewRelease(tex->view);
        wgpuTextureRelease(tex->texture);
        free(tex->pixels);
        free(tex);

        return NULL;
    }

    texturePool.textures = temp;
    texturePool.textures[texturePool.count++] = tex;

    vanir_log_info("textureUpload: \"%s\" %ux%u ready", name, w, h);

    return tex;
}

/* ↓ free all GPU resources for a texture and remove it from the pool ↓ */
void textureRelease(struct Texture *tex) {
    for (int i = 0; i < texturePool.count; ++i) {
        if (texturePool.textures[i] != tex)
            continue;

        vanir_log_info("texture.release: \"%s\"", tex->name);

        if (activeTexture == tex)
            activeTexture = NULL;

        if (tex->bindGroup)       { wgpuBindGroupRelease(tex->bindGroup);             tex->bindGroup       = NULL; }
        if (tex->bindGroupLayout) { wgpuBindGroupLayoutRelease(tex->bindGroupLayout); tex->bindGroupLayout = NULL; }
        if (tex->sampler)         { wgpuSamplerRelease(tex->sampler);                 tex->sampler         = NULL; }
        if (tex->view)            { wgpuTextureViewRelease(tex->view);                tex->view            = NULL; }
        if (tex->texture)         { wgpuTextureRelease(tex->texture);                 tex->texture         = NULL; }
        if (tex->pixels)          { free(tex->pixels);                                tex->pixels          = NULL; }

        free(tex);

        /* ↓ compact pool ↓ */
        texturePool.textures[i] = texturePool.textures[--texturePool.count];

        return;
    }

    throw("texture.release", "texture", "texture not found in pool");
}

/* ↓ load a new image from path into an existing texture; replaces GPU resources in-place ↓ */
/* ↓ also updates tex->path so future :setImage() calls without args can use it ↓ */
/* ↓ returns true on success; tex pointer stays valid ↓ */
bool textureSetImage(struct Texture *tex, const char *path) {
    int w, h, channels;
    uint8_t *pixels = stbi_load(path, &w, &h, &channels, 4);

    if (!pixels) {
        throw("texture:setImage", tex->name, stbi_failure_reason());

        return false;
    }

    /* ↓ get file size ↓ */
    size_t fileSize = 0;
    FILE *f = fopen(path, "rb");

    if (f) {
        fseek(f, 0, SEEK_END);

        fileSize = (size_t)ftell(f);

        fclose(f);
    }

    WGPUTextureFormat format = (windowPool.count > 0)
        ? windowPool.windows[0]->surfaceFormat
        : WGPUTextureFormat_BGRA8Unorm;

    for (uint32_t i = 0; i < (uint32_t)(w * h); ++i) {
        uint8_t r = pixels[i * 4 + 0];
        pixels[i * 4 + 0] = pixels[i * 4 + 2];
        pixels[i * 4 + 2] = r;
    }

    /* ↓ drop old GPU resources ↓ */
    if (tex->bindGroup)       { wgpuBindGroupRelease(tex->bindGroup);             tex->bindGroup       = NULL; }
    if (tex->bindGroupLayout) { wgpuBindGroupLayoutRelease(tex->bindGroupLayout); tex->bindGroupLayout = NULL; }
    if (tex->sampler)         { wgpuSamplerRelease(tex->sampler);                 tex->sampler         = NULL; }
    if (tex->view)            { wgpuTextureViewRelease(tex->view);                tex->view            = NULL; }
    if (tex->texture)         { wgpuTextureRelease(tex->texture);                 tex->texture         = NULL; }
    if (tex->pixels)          { free(tex->pixels);                                tex->pixels          = NULL; }

    tex->path = path;
    tex->width = (uint32_t)w;
    tex->height = (uint32_t)h;
    tex->channels = channels;
    tex->fileSize = fileSize;
    tex->format = format;

    /* ↓ re-upload ↓ */
    tex->pixels = malloc((size_t)(4 * w * h));

    if (!tex->pixels) {
        stbi_image_free(pixels);
        throw("texture:setImage", tex->name, "malloc failed");

        return false;
    }

    memcpy(tex->pixels, pixels, (size_t)(4 * w * h));
    stbi_image_free(pixels);

    WGPUTextureDescriptor tex_desc = {0};
    tex_desc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    tex_desc.dimension = WGPUTextureDimension_2D;
    tex_desc.size = (WGPUExtent3D){ tex->width, tex->height, 1 };
    tex_desc.format = tex->format;
    tex_desc.mipLevelCount = 1;
    tex_desc.sampleCount = 1;
    tex->texture = wgpuDeviceCreateTexture(gpu.device, &tex_desc);

    if (!tex->texture) {
        throw("texture:setImage", tex->name, "wgpuDeviceCreateTexture failed");

        return false;
    }

    tex->view = wgpuTextureCreateView(tex->texture, NULL);

    if (!tex->view) {
        throw("texture:setImage", tex->name, "wgpuTextureCreateView failed");
        wgpuTextureRelease(tex->texture);

        return false;
    }

    WGPUTexelCopyTextureInfo dst_info = {0};
    dst_info.texture = tex->texture;
    dst_info.mipLevel = 0;
    dst_info.origin = (WGPUOrigin3D){ 0, 0, 0 };
    dst_info.aspect = WGPUTextureAspect_All;

    WGPUTexelCopyBufferLayout layout = {0};
    layout.offset = 0;
    layout.bytesPerRow = 4 * tex->width;
    layout.rowsPerImage = tex->height;

    WGPUExtent3D extent = { tex->width, tex->height, 1 };

    wgpuQueueWriteTexture(gpu.queue, &dst_info, tex->pixels, (size_t)(4 * tex->width * tex->height), &layout, &extent);

    if (!buildTextureBindGroup(tex)) {
        wgpuTextureViewRelease(tex->view);
        wgpuTextureRelease(tex->texture);

        return false;
    }

    vanir_log_info("texture:setImage: \"%s\" loaded \"%s\" %ux%u ready", tex->name, path, tex->width, tex->height);

    return true;
}

/* ↓ get a pixel color from cpu-side pixel data ↓ */
bool textureGetColor(struct Texture *tex, uint32_t x, uint32_t y, float *r, float *g, float *b, float *a) {
    if (!tex->pixels)
        return false;

    if (x >= tex->width || y >= tex->height)
        return false;

    uint32_t i = (y * tex->width + x) * 4;
    uint8_t c0 = tex->pixels[i + 0];
    uint8_t c1 = tex->pixels[i + 1];
    uint8_t c2 = tex->pixels[i + 2];
    uint8_t  ca = tex->pixels[i + 3];

    if (tex->format == WGPUTextureFormat_BGRA8Unorm) {
        *r = (float)c2;
        *g = (float)c1;
        *b = (float)c0;
    } else {
        *r = (float)c0;
        *g = (float)c1;
        *b = (float)c2;
    }

    *a = (float)ca;

    return true;
}

/* ↓ draw a texture (or sub-region) as a quad using the textured pipeline ↓ */
/* ↓ drawTexturedRect(sx, sy, sw, sh, dx, dy [, dw, dh]) ↓ */
/* ↓ sx/sy/sw/sh = source region on the texture; dx/dy = destination on screen; dw/dh default to sw/sh ↓ */
int drawTexturedRect(lua_State *L) {
    struct Texture *tex = activeTexture;

    if (!tex) {
        throw("drawTexturedRect", "texture", "no active texture — call render.setTexture first");

        return 0;
    }

    struct glfwWindow *w = currentRenderWindow;

    if (!w || !w->frame.encoder || !w->frame.passEncoder) {
        throw("drawTexturedRect", tex->name, "no active render frame");

        return 0;
    }

    float sx = (float)luaL_optnumber(L, 1, 0);
    float sy = (float)luaL_optnumber(L, 2, 0);
    float sw = (float)luaL_optnumber(L, 3, (lua_Number)tex->width);
    float sh = (float)luaL_optnumber(L, 4, (lua_Number)tex->height);
    float dx = (float)luaL_optnumber(L, 5, 0);
    float dy = (float)luaL_optnumber(L, 6, 0);
    float dw = (float)luaL_optnumber(L, 7, sw);
    float dh = (float)luaL_optnumber(L, 8, sh);

    /* ↓ compute normalised UV coords from source rect ↓ */
    float u0 = sx / (float)tex->width;
    float v0 = sy / (float)tex->height;
    float u1 = (sx + sw) / (float)tex->width;
    float v1 = (sy + sh) / (float)tex->height;

    vanir_log_info("drawTexturedRect: \"%s\" src=(%.0f,%.0f,%.0f,%.0f) dst=(%.0f,%.0f,%.0f,%.0f)",
        tex->name, sx, sy, sw, sh, dx, dy, dw, dh);

    drawTexturedQuadImmediate(w, tex, dx, dy, dw, dh, u0, v0, u1, v1);

    return 0;
}

/* ↓ textures.load(path [, name]) → loads using stb_image; name defaults to path ↓ */
/* ↓ requires stb_image.h in src/graphics/ and VANIR_HAS_STB_IMAGE compile flag ↓ */
int luaTextureLoad(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const char *name = luaL_optstring(L, 2, path);

    struct Texture *existing = findTexture(name);

    if (existing) {
        vanir_log_info("texture.load: \"%s\" already loaded, returning existing", name);
        pushTexture(L, existing);

        return 1;
    }

    int w, h, channels;
    uint8_t *pixels = stbi_load(path, &w, &h, &channels, 4);

    if (!pixels) {
        throw("texture.create", name, stbi_failure_reason());
        lua_pushnil(L);

        return 1;
    }

    /* ↓ get file size ↓ */
    size_t fileSize = 0;
    FILE *f = fopen(path, "rb");

    if (f) {
        fseek(f, 0, SEEK_END);
        fileSize = (size_t)ftell(f);
        fclose(f);
    }

    WGPUTextureFormat format = (windowPool.count > 0)
        ? windowPool.windows[0]->surfaceFormat
        : WGPUTextureFormat_BGRA8Unorm;

    for (uint32_t i = 0; i < (uint32_t)(w * h); ++i) {
        uint8_t r = pixels[i * 4 + 0];
        pixels[i * 4 + 0] = pixels[i * 4 + 2];
        pixels[i * 4 + 2] = r;
    }

    struct Texture *tex = textureUpload(name, pixels, (uint32_t)w, (uint32_t)h);
    stbi_image_free(pixels);

    if (tex) {
        tex->path     = path;
        tex->channels = channels;
        tex->fileSize = fileSize;
        vanir_log_info("texture.load: \"%s\" loaded \"%s\" %dx%d channels=%d size=%zu",
            name, path, w, h, channels, fileSize);
    }

    pushTexture(L, tex);

    return 1;
}

/* ↓ textures.loadFromPixels(name, width, height, pixels_string) ↓ */
/* ↓ pixels_string is a raw RGBA8 byte string; width*height*4 bytes ↓ */
int luaTextureLoadFromPixels(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    int w = (int)luaL_checkinteger(L, 2);
    int h = (int)luaL_checkinteger(L, 3);

    size_t len;
    const uint8_t *pixels = (const uint8_t *)luaL_checklstring(L, 4, &len);

    if ((size_t)(w * h * 4) != len) {
        throw("texture.loadFromPixels", name, "pixel data length does not match width*height*4");
        lua_pushnil(L);

        return 1;
    }

    uint8_t *converted = malloc(len);

    if (!converted) {
        throw("texture.loadFromPixels", name, "malloc failed");
        lua_pushnil(L);

        return 1;
    }

    memcpy(converted, pixels, len);

    for (uint32_t i = 0; i < (uint32_t)(w * h); ++i) {
        uint8_t r = converted[i * 4 + 0];
        converted[i * 4 + 0] = converted[i * 4 + 2];
        converted[i * 4 + 2] = r;
    }

    struct Texture *tex = textureUpload(name, converted, (uint32_t)w, (uint32_t)h);
    free(converted);

    /* ↓ path stays NULL; channels/fileSize stay 0 — pixel data was provided directly ↓ */

    if (tex)
        vanir_log_info("texture.loadFromPixels: \"%s\" %dx%d ready", name, w, h);

    pushTexture(L, tex);

    return 1;
}

/* ↓ textures.getSize(tex) → width, height  (module-level convenience) ↓ */
int luaTextureGetSize(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_pushinteger(L, (lua_Integer)tex->width);
    lua_pushinteger(L, (lua_Integer)tex->height);

    return 2;
}

void destroyAllTextures(void) {
    vanir_log_info("destroyAllTextures: releasing %d textures", texturePool.count);

    for (int i = 0; i < texturePool.count; ++i) {
        struct Texture *tex = texturePool.textures[i];

        vanir_log_info("destroyAllTextures: releasing \"%s\"", tex->name);

        if (tex->bindGroup)       { wgpuBindGroupRelease(tex->bindGroup);             tex->bindGroup       = NULL; }
        if (tex->bindGroupLayout) { wgpuBindGroupLayoutRelease(tex->bindGroupLayout); tex->bindGroupLayout = NULL; }
        if (tex->sampler)         { wgpuSamplerRelease(tex->sampler);                 tex->sampler         = NULL; }
        if (tex->view)            { wgpuTextureViewRelease(tex->view);                tex->view            = NULL; }
        if (tex->texture)         { wgpuTextureRelease(tex->texture);                 tex->texture         = NULL; }
        if (tex->pixels)          { free(tex->pixels);                                tex->pixels          = NULL; }

        free(tex);
    }

    free(texturePool.textures);

    texturePool.textures = NULL;
    texturePool.count    = 0;
}

static const luaL_Reg luaTextures[] = {
    {"create", luaTextureLoad},
    {"load", luaTextureLoad},
    {"loadFromPixels", luaTextureLoadFromPixels},
    {"getSize", luaTextureGetSize},

    {NULL, NULL}
};

int texturesInit(lua_State *L) {
    luaL_newlib(L, luaTextures);

    return 1;
}