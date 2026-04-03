#ifndef COMMON_H
#define COMMON_H

#include "../lua_config.h"
#include "../vanir.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

struct Texture;

/* ↓ luas color type disassembles to this struct ↓ */
struct color {
    float r, g, b, a;
};

/* ↓ a file handle; stored as lightuserdata inside the lua table ↓ */
struct File {
    FILE *handle;
    char *path;
};

/* ↓ userdata structs for value types — one allocation per object, no table overhead ↓ */
struct VanirVec  { float x, y, z; };
struct VanirAng  { float p, y, r; };
struct VanirQuat { float x, y, z, w; };
struct VanirCol  { float r, g, b, a; };

/* ↓ userdata push helpers ↓ */
struct VanirAng *pushAngle(lua_State *L, float p, float y, float r);
struct VanirCol *pushColor(lua_State *L, float r, float g, float b, float a);
struct VanirQuat *pushQuat(lua_State *L, float x, float y, float z, float w);
struct VanirVec *pushVec(lua_State *L, float x, float y, float z);

/* ↓ internal helpers ↓ */
static inline void pushRegList(lua_State *L, const luaL_Reg *reg) {
    for (; reg->name != NULL; ++reg) {
        lua_pushcfunction(L, reg->func);
        lua_setfield(L, -2, reg->name);
    }
}

static inline float getfieldf(lua_State *L, int idx, const char *key) {
    lua_getfield(L, idx, key);
    float v = (float)lua_tonumber(L, -1);
    
    lua_pop(L, 1);
    
    return v;
}

/* ↓ addMethods: original table-based __index helper, used by window and Matrix ↓ */
static inline void addMethods(lua_State *L, const char *name, const luaL_Reg *methods, const luaL_Reg *meta) {
    luaL_newmetatable(L, name);

    if (methods) {
        lua_newtable(L);
        pushRegList(L, methods);
        lua_setfield(L, -2, "__index");
    }

    if (meta)
        pushRegList(L, meta);

    lua_setmetatable(L, -2);
}

/* ↓ addMethodsUD: userdata variant — stores methods in __methods for custom __index handlers ↓ */
/* ↓ used by Vector, Angle, Quaternion, Color whose __index handles field access first          ↓ */
static inline void addMethodsUD(lua_State *L, const char *name, const luaL_Reg *methods, const luaL_Reg *meta) {
    if (luaL_newmetatable(L, name)) {
        /* ↓ first registration: populate the metatable ↓ */
        if (methods) {
            lua_newtable(L);
            pushRegList(L, methods);
            lua_setfield(L, -2, "__methods");
        }

        if (meta)
            pushRegList(L, meta);
    }

    lua_setmetatable(L, -2);
}

static inline float degToRad(float d) {
    return d * (float)(M_PI / 180.0);
}

static inline float radToDeg(float r) {
    return r * (float)(180.0 / M_PI);
}

/* ↓ typed userdata getters — check metatable and return pointer ↓ */
static inline struct VanirVec *checkVec(lua_State *L, int idx) {
    return (struct VanirVec *)luaL_checkudata(L, idx, "vanir.Vector");
}

static inline struct VanirAng *checkAng(lua_State *L, int idx) {
    return (struct VanirAng *)luaL_checkudata(L, idx, "vanir.Angle");
}

static inline struct VanirQuat *checkQuat(lua_State *L, int idx) {
    return (struct VanirQuat *)luaL_checkudata(L, idx, "vanir.Quaternion");
}

static inline struct VanirCol *checkCol(lua_State *L, int idx) {
    return (struct VanirCol *)luaL_checkudata(L, idx, "vanir.Color");
}

/* ↓ public API declarations ↓ */
int Vector(lua_State *L);
int Angle(lua_State *L);
int Color(lua_State *L);
int Quaternion(lua_State *L);
int Matrix(lua_State *L);

/* ↓ matrix helpers used by render.c for the matrix stack ↓ */
void matrixGet(lua_State *L, int idx, float m[9]);
void matrixPush(lua_State *L, const float m[9]);
void matrixIdentity(float m[9]);
void matrixTransformPoint(const float m[9], float x, float y, float *ox, float *oy);

void pushTexture(lua_State *L, struct Texture *tex);
struct Texture *getTexture(lua_State *L, int idx);

void pushFile(lua_State *L, FILE *handle, const char *path);
struct File *getFile(lua_State *L, int idx);

/* ↓ shared field-access helper for userdata value types ↓ */

/* ↓ vanirUD_indexFallback: look up key in the __methods sub-table of a named metatable ↓ */
/* ↓ leaves one value on the stack (the method or nil)                                  ↓ */
static inline void vanirUD_indexFallback(lua_State *L, const char *metatableName, const char *key) {
    luaL_getmetatable(L, metatableName);
    lua_getfield(L, -1, "__methods");

    if (!lua_isnil(L, -1)) {
        lua_getfield(L, -1, key);
        lua_remove(L, -2);   /* ↓ pop __methods, keep result ↓ */
        lua_remove(L, -2);   /* ↓ pop metatable ↓ */
    } else {
        lua_pop(L, 2);       /* ↓ pop nil __methods + metatable ↓ */
        lua_pushnil(L);
    }
}

#endif