#pragma once

#include "lua_config.h"
#include <stdio.h>

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

int Vector(lua_State *L);
int Angle(lua_State *L);
int Color(lua_State *L);
int Quaternion(lua_State *L);

/* ↓ texture lua object helpers ↓ */
void pushTexture(lua_State *L, struct Texture *tex);
struct Texture *getTexture(lua_State *L, int idx);

/* ↓ file lua object helpers ↓ */
void pushFile(lua_State *L, FILE *handle, const char *path);
struct File *getFile(lua_State *L, int idx);
