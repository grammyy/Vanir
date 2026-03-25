#pragma once

#include "lua_config.h"

struct Texture;

/* ↓ luas color type disassembles to this struct ↓ */
struct color {
    float r, g, b, a;
};

int Vector(lua_State *L);
int Angle(lua_State *L);
int Color(lua_State *L);

/* ↓ texture lua object helpers ↓ */
void pushTexture(lua_State *L, struct Texture *tex);
struct Texture *getTexture(lua_State *L, int idx);
