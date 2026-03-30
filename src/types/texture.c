/* texture.c */
#include "common.h"
#include "../graphics/textures.h"
#include "../modules/render.h"

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
    {"getWidth",  textureGetWidth},
    {"getHeight", textureGetHeight},
    {"getSize",   textureGetSize},
    {"getColor",  textureMetaGetColor},
    {"draw",      textureDraw},
    {"release",   textureReleaseMeta},
    {"setImage",  textureSetImageMeta},

    {NULL, NULL}
};

static const luaL_Reg textureMeta[] = {
    {"__tostring", toStringTexture},

    {NULL, NULL}
};