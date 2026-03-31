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

static inline float degToRad(float d) {
    return d * (float)(M_PI / 180.0);
}

static inline float radToDeg(float r) {
    return r * (float)(180.0 / M_PI);
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

#endif
