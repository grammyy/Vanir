/* texture.c */
#include "common.h"
#include "../graphics/textures.h"
#include "../modules/render.h"
#include "../modules/windows.h"

static const luaL_Reg textureMethods[];
static const luaL_Reg textureMeta[];

void pushTexture(lua_State *L, struct Texture *tex) {
    if (!tex) {
        lua_pushnil(L);

        return;
    }

    lua_newtable(L);
    lua_pushlightuserdata(L, tex);
    lua_setfield(L, -2, "__ptr");

    /* ↓ bake plain read-only fields directly onto the table ↓ */
    lua_pushstring(L, tex->name ? tex->name : "");
    lua_setfield(L, -2, "name");

    lua_pushstring(L, tex->path ? tex->path : "");
    lua_setfield(L, -2, "path");

    lua_pushinteger(L, (lua_Integer)tex->channels);
    lua_setfield(L, -2, "channels");

    lua_pushinteger(L, (lua_Integer)tex->fileSize);
    lua_setfield(L, -2, "fileSize");

    addMethods(L, "vanir.Texture", textureMethods, textureMeta);
}

struct Texture *getTexture(lua_State *L, int idx) {
    struct Texture *tex;

    luaL_checktype(L, idx, LUA_TTABLE);
    lua_getfield(L, idx, "__ptr");

    tex = (struct Texture *)lua_touserdata(L, -1);

    lua_pop(L, 1);

    if (!tex)
        luaL_error(L, "expected texture object");

    return tex;
}

static int toStringTexture(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_pushfstring(L, "Texture: %s (%d x %d)", tex->name ? tex->name : "(unnamed)", (int)tex->width, (int)tex->height);

    return 1;
}

/* ↓ tex:getWidth() → width ↓ */
static int textureGetWidth(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_pushinteger(L, (lua_Integer)tex->width);

    return 1;
}

/* ↓ tex:getHeight() → height ↓ */
static int textureGetHeight(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_pushinteger(L, (lua_Integer)tex->height);

    return 1;
}

/* ↓ tex:getSize() → width, height ↓ */
static int textureGetSize(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_pushinteger(L, (lua_Integer)tex->width);
    lua_pushinteger(L, (lua_Integer)tex->height);

    return 2;
}

/* ↓ tex:getColor(x, y) → Color ↓ */
static int textureMetaGetColor(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);
    uint32_t x = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t y = (uint32_t)luaL_checkinteger(L, 3);

    float r, g, b, a;

    if (!tex->pixels) {
        throw("texture:getColor", tex->name, "no cpu pixel data available");
        lua_pushnil(L);

        return 1;
    }

    if (!textureGetColor(tex, x, y, &r, &g, &b, &a)) {
        throw("texture:getColor", tex->name, "pixel coordinates out of range");
        lua_pushnil(L);

        return 1;
    }

    lua_getglobal(L, "Color");

    if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, r);
        lua_pushnumber(L, g);
        lua_pushnumber(L, b);
        lua_pushnumber(L, a);
        lua_call(L, 4, 1);
        return 1;
    }

    lua_pop(L, 1);

    lua_newtable(L);
    setFieldNumber(L, "r", r);
    setFieldNumber(L, "g", g);
    setFieldNumber(L, "b", b);
    setFieldNumber(L, "a", a);

    return 1;
}

/* ↓ tex:getName() → string ↓ */
static int textureGetName(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_pushstring(L, tex->name ? tex->name : "");

    return 1;
}

/* ↓ tex:getShader() → string; returns the effective pipeline/shader name for this texture ↓ */
/* ↓ Vanir uses a single built-in textured pipeline, so this always returns "Textured" ↓ */
static int textureGetShader(lua_State *L) {
    (void)getTexture(L, 1);

    lua_pushstring(L, "Textured");

    return 1;
}

/* ↓ tex:getKeyValues() → table; returns a table of texture metadata as key-value pairs ↓ */
static int textureGetKeyValues(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_newtable(L);

    lua_pushstring(L, tex->name ? tex->name : "");
    lua_setfield(L, -2, "$basetexture");

    lua_pushinteger(L, (lua_Integer)tex->width);
    lua_setfield(L, -2, "width");

    lua_pushinteger(L, (lua_Integer)tex->height);
    lua_setfield(L, -2, "height");

    lua_pushinteger(L, (lua_Integer)tex->channels);
    lua_setfield(L, -2, "channels");

    return 1;
}

/* ↓ tex:getFloat(key) → number; stub — no float parameter store yet, always returns 0 ↓ */
static int textureGetFloat(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);

    lua_pushnumber(L, 0);

    return 1;
}

/* ↓ tex:getInt(key) → number; stub — always returns 0 ↓ */
static int textureGetInt(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);

    lua_pushinteger(L, 0);

    return 1;
}

/* ↓ tex:getMatrix(key) → nil; stub — no matrix parameter store ↓ */
static int textureGetMatrix(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);

    lua_pushnil(L);

    return 1;
}

/* ↓ tex:getString(key) → string; returns name for "$basetexture", nil for others ↓ */
static int textureGetString(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);
    const char *key     = luaL_checkstring(L, 2);

    if (strcmp(key, "$basetexture") == 0) {
        lua_pushstring(L, tex->name ? tex->name : "");
    } else if (strcmp(key, "path") == 0) {
        lua_pushstring(L, tex->path ? tex->path : "");
    } else {
        lua_pushnil(L);
    }

    return 1;
}

/* ↓ tex:getTexture(key) → tex; returns self for "$basetexture" ↓ */
static int textureGetTextureParm(lua_State *L) {
    (void)luaL_checkstring(L, 2);

    /* ↓ just return self — the caller already has the texture object ↓ */
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ tex:getVector(key) → Vector | nil; stub ↓ */
static int textureGetVector(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);

    lua_pushnil(L);

    return 1;
}

/* ↓ tex:getVectorLinear(key) → Vector | nil; stub ↓ */
static int textureGetVectorLinear(lua_State *L) {
    return textureGetVector(L);
}

/* ↓ tex:setFloat(key, value) → nil; stub ↓ */
static int textureSetFloat(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);
    (void)luaL_checknumber(L, 3);

    return 0;
}

/* ↓ tex:setInt(key, value) → nil; stub ↓ */
static int textureSetInt(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);
    (void)luaL_checkinteger(L, 3);

    return 0;
}

/* ↓ tex:setMatrix(key, matrix) → nil; stub ↓ */
static int textureSetMatrix(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);

    return 0;
}

/* ↓ tex:setString(key, value) → nil; stub ↓ */
static int textureSetString(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);
    (void)luaL_checkstring(L, 3);

    return 0;
}

/* ↓ tex:setTexture(key, tex2) → nil; swaps the underlying GPU data from another texture object ↓ */
/* ↓ currently a stub — in-place GPU data swapping is non-trivial ↓ */
static int textureSetTexture(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);

    return 0;
}

/* ↓ tex:setTextureURL(key, url) → nil; stub — no HTTP fetching in Vanir ↓ */
static int textureSetTextureURL(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);
    (void)luaL_checkstring(L, 3);

    return 0;
}

/* ↓ tex:setTextureRenderTarget(key, rt) → nil; stub ↓ */
static int textureSetTextureRenderTarget(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);

    return 0;
}

/* ↓ tex:setUndefined(key) → nil; clears a parameter; stub ↓ */
static int textureSetUndefined(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);

    return 0;
}

/* ↓ tex:setVector(key, vec) → nil; stub ↓ */
static int textureSetVector(lua_State *L) {
    (void)getTexture(L, 1);
    (void)luaL_checkstring(L, 2);

    return 0;
}

/* ↓ tex:recompute() → nil; rebuilds GPU bind group from current pixel data ↓ */
static int textureRecompute(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    if (!tex->pixels || !tex->texture)
        return 0;

    /* ↓ re-upload cpu pixels to the existing gpu texture ↓ */
    extern struct VanirGPU gpu;

    WGPUTexelCopyTextureInfo dst_info = {0};
    dst_info.texture  = tex->texture;
    dst_info.mipLevel = 0;
    dst_info.origin   = (WGPUOrigin3D){ 0, 0, 0 };
    dst_info.aspect   = WGPUTextureAspect_All;

    WGPUTexelCopyBufferLayout layout = {0};
    layout.offset       = 0;
    layout.bytesPerRow  = 4 * tex->width;
    layout.rowsPerImage = tex->height;

    WGPUExtent3D extent = { tex->width, tex->height, 1 };

    wgpuQueueWriteTexture(gpu.queue, &dst_info, tex->pixels, (size_t)(4 * tex->width * tex->height), &layout, &extent);

    return 0;
}

/* ↓ tex:draw(sx, sy, sw, sh, dx, dy [, dw, dh]) ↓ */
/* ↓ temporarily sets this texture as active so drawTexturedRect can find it, then restores the previous one ↓ */
static int textureDraw(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    struct Texture *prev = activeTexture;
    activeTexture = tex;

    /* ↓ shift args down: remove self from stack so drawTexturedRect sees sx at 1 ↓ */
    lua_remove(L, 1);

    int ret = drawTexturedRect(L);

    activeTexture = prev;

    return ret;
}

/* ↓ tex:release() — frees GPU resources and invalidates this object ↓ */
static int textureReleaseMeta(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    textureRelease(tex);

    lua_pushnil(L);
    lua_setfield(L, 1, "__ptr");

    return 0;
}

/* ↓ tex:destroy() — alias of release ↓ */
static int textureDestroyMeta(lua_State *L) {
    return textureReleaseMeta(L);
}

/* ↓ tex:setImage(path) — loads a new image into this texture from a file ↓ */
/* ↓ path defaults to tex.path so tex:setImage() reloads the original file ↓ */
static int textureSetImageMeta(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    /* ↓ default to the texture's current path so :setImage() acts as a reload ↓ */
    const char *path = luaL_optstring(L, 2, tex->path);

    if (!path) {
        throw("texture:setImage", tex->name, "no path — provide a path or load from a file first");
        lua_pushboolean(L, 0);
        return 1;
    }

    if (!textureSetImage(tex, path)) {
        lua_pushboolean(L, 0);
        return 1;
    }

    /* ↓ update the baked fields to reflect new image ↓ */
    lua_pushstring(L, tex->path);
    lua_setfield(L, 1, "path");

    lua_pushinteger(L, (lua_Integer)tex->channels);
    lua_setfield(L, 1, "channels");

    lua_pushinteger(L, (lua_Integer)tex->fileSize);
    lua_setfield(L, 1, "fileSize");

    lua_pushboolean(L, 1);

    return 1;
}

static const luaL_Reg textureMethods[] = {
    /* ↓ size queries ↓ */
    {"getWidth",               textureGetWidth},
    {"getHeight",              textureGetHeight},
    {"getSize",                textureGetSize},

    /* ↓ pixel data ↓ */
    {"getColor",               textureMetaGetColor},

    /* ↓ material-style getters ↓ */
    {"getName",                textureGetName},
    {"getShader",              textureGetShader},
    {"getKeyValues",           textureGetKeyValues},
    {"getFloat",               textureGetFloat},
    {"getInt",                 textureGetInt},
    {"getMatrix",              textureGetMatrix},
    {"getString",              textureGetString},
    {"getTexture",             textureGetTextureParm},
    {"getVector",              textureGetVector},
    {"getVectorLinear",        textureGetVectorLinear},

    /* ↓ material-style setters (mutable) ↓ */
    {"setFloat",               textureSetFloat},
    {"setInt",                 textureSetInt},
    {"setMatrix",              textureSetMatrix},
    {"setString",              textureSetString},
    {"setTexture",             textureSetTexture},
    {"setTextureURL",          textureSetTextureURL},
    {"setTextureRenderTarget", textureSetTextureRenderTarget},
    {"setUndefined",           textureSetUndefined},
    {"setVector",              textureSetVector},
    {"recompute",              textureRecompute},

    /* ↓ lifecycle ↓ */
    {"draw",                   textureDraw},
    {"setImage",               textureSetImageMeta},
    {"release",                textureReleaseMeta},
    {"destroy",                textureDestroyMeta},

    {NULL, NULL}
};

static const luaL_Reg textureMeta[] = {
    {"__tostring", toStringTexture},

    {NULL, NULL}
};