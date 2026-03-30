#include "common.h"

#include <math.h>
#include <string.h>

/* ↓ 2D affine 3×3 matrix, stored row-major in a 9-element flat table (1-indexed in lua) ↓ */
/* ↓ layout:                                                                               ↓ */
/*   | m[1]  m[2]  m[3] |   [ cosθ  -sinθ   tx ]                                          */
/*   | m[4]  m[5]  m[6] |   [ sinθ   cosθ   ty ]                                          */
/*   | m[7]  m[8]  m[9] |   [  0      0      1 ]                                          */
/*                                                                                         */
/* ↓ mutation semantics: :rotate/:translate/:scale mutate the table IN-PLACE and return   ↓ */
/* ↓ self so chaining works — mat:translate(10,0):rotate(45)                              ↓ */

static const luaL_Reg matMethods[];
static const luaL_Reg matMeta[];

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

/* ↓ pull all 9 elements off a table at stack index idx ↓ */
static void matGet(lua_State *L, int idx, float m[9]) {
    for (int i = 0; i < 9; i++) {
        lua_rawgeti(L, idx, i + 1);
        m[i] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
}

/* ↓ write 9 elements back into the table at stack index idx (must already exist) ↓ */
static void matSet(lua_State *L, int idx, const float m[9]) {
    /* ↓ normalise negative indices before pushing ↓ */
    if (idx < 0) idx = lua_gettop(L) + idx + 1;

    for (int i = 0; i < 9; i++) {
        lua_pushnumber(L, m[i]);
        lua_rawseti(L, idx, i + 1);
    }
}

/* ↓ push a brand-new matrix table; used by the Matrix() constructor and read-only ops ↓ */
static void matPush(lua_State *L, const float m[9]) {
    lua_createtable(L, 9, 0);

    for (int i = 0; i < 9; i++) {
        lua_pushnumber(L, m[i]);
        lua_rawseti(L, -2, i + 1);
    }

    addMethods(L, "vanir.Matrix", matMethods, matMeta);
}

/* ↓ multiply two 3×3 matrices: out = a * b ↓ */
static void mat3Mul(const float a[9], const float b[9], float out[9]) {
    out[0] = a[0]*b[0] + a[1]*b[3] + a[2]*b[6];
    out[1] = a[0]*b[1] + a[1]*b[4] + a[2]*b[7];
    out[2] = a[0]*b[2] + a[1]*b[5] + a[2]*b[8];

    out[3] = a[3]*b[0] + a[4]*b[3] + a[5]*b[6];
    out[4] = a[3]*b[1] + a[4]*b[4] + a[5]*b[7];
    out[5] = a[3]*b[2] + a[4]*b[5] + a[5]*b[8];

    out[6] = a[6]*b[0] + a[7]*b[3] + a[8]*b[6];
    out[7] = a[6]*b[1] + a[7]*b[4] + a[8]*b[7];
    out[8] = a[6]*b[2] + a[7]*b[5] + a[8]*b[8];
}

static void mat3Identity(float m[9]) {
    m[0]=1; m[1]=0; m[2]=0;
    m[3]=0; m[4]=1; m[5]=0;
    m[6]=0; m[7]=0; m[8]=1;
}

/* ↓ __tostring ↓ */
static int matToString(lua_State *L) {
    float m[9];
    matGet(L, 1, m);

    lua_pushfstring(L,
        "| %.3f %.3f %.3f | %.3f %.3f %.3f | %.3f %.3f %.3f |",
        m[0], m[1], m[2],
        m[3], m[4], m[5],
        m[6], m[7], m[8]);

    return 1;
}

/* ↓ __mul: Matrix * Matrix  or  Matrix * Vector ↓ */
static int matMul(lua_State *L) {
    float a[9];
    matGet(L, 1, a);

    /* ↓ Matrix * Vector ↓ */
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "x");
        lua_getfield(L, 2, "y");

        if (!lua_isnil(L, -2) && !lua_isnil(L, -1)) {
            float vx = (float)lua_tonumber(L, -2);
            float vy = (float)lua_tonumber(L, -1);
            lua_pop(L, 2);

            lua_newtable(L);
            setFieldNumber(L, "x", a[0]*vx + a[1]*vy + a[2]);
            setFieldNumber(L, "y", a[3]*vx + a[4]*vy + a[5]);
            setFieldNumber(L, "z", 0.0f);

            luaL_getmetatable(L, "vanir.Vector");
            if (!lua_isnil(L, -1))
                lua_setmetatable(L, -2);
            else
                lua_pop(L, 1);

            return 1;
        }

        lua_pop(L, 2);
    }

    /* ↓ Matrix * Matrix → new matrix ↓ */
    float b[9], out[9];
    matGet(L, 2, b);
    mat3Mul(a, b, out);
    matPush(L, out);

    return 1;
}

static int matEq(lua_State *L) {
    float a[9], b[9];
    matGet(L, 1, a);
    matGet(L, 2, b);

    for (int i = 0; i < 9; i++) {
        if (a[i] != b[i]) { lua_pushboolean(L, 0); return 1; }
    }

    lua_pushboolean(L, 1);
    return 1;
}

/* ↓ :translate(dx, dy)                                                         ↓ */
/* ↓ mutates self in-place, returns self so chaining works                      ↓ */
static int matTranslate(lua_State *L) {
    float m[9];
    matGet(L, 1, m);

    float dx = (float)luaL_checknumber(L, 2);
    float dy = (float)luaL_optnumber(L, 3, 0.0);

    float t[9] = { 1,0,dx, 0,1,dy, 0,0,1 };
    float out[9];
    mat3Mul(m, t, out);

    matSet(L, 1, out);


    lua_pushvalue(L, 1);    /* ↓ return self ↓ */
    return 1;
}

/* ↓ :rotate(degrees_or_Angle)                                                  ↓ */
/* ↓ mutates self in-place, returns self                                        ↓ */
static int matRotate(lua_State *L) {
    float m[9];
    matGet(L, 1, m);

    float deg;

    if (lua_isnumber(L, 2)) {
        deg = (float)lua_tonumber(L, 2);
    } else {
        lua_getfield(L, 2, "yaw");
        deg = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    float rad = deg * (float)(M_PI / 180.0);
    float c   = cosf(rad);
    float s   = sinf(rad);

    float r[9] = { c,-s,0, s,c,0, 0,0,1 };
    float out[9];
    mat3Mul(m, r, out);

    matSet(L, 1, out);


    lua_pushvalue(L, 1);    /* ↓ return self ↓ */
    return 1;
}

/* ↓ :scale(sx [, sy])                                                          ↓ */
/* ↓ mutates self in-place, returns self                                        ↓ */
static int matScale(lua_State *L) {
    float m[9];
    matGet(L, 1, m);

    float sx = (float)luaL_checknumber(L, 2);
    float sy = (float)luaL_optnumber(L, 3, sx);

    float s[9] = { sx,0,0, 0,sy,0, 0,0,1 };
    float out[9];
    mat3Mul(m, s, out);

    matSet(L, 1, out);


    lua_pushvalue(L, 1);    /* ↓ return self ↓ */
    return 1;
}

/* ↓ :transformPoint(x, y) → rx, ry — read-only, returns two numbers ↓ */
static int matTransformPoint(lua_State *L) {
    float m[9];
    matGet(L, 1, m);

    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);

    lua_pushnumber(L, m[0]*x + m[1]*y + m[2]);
    lua_pushnumber(L, m[3]*x + m[4]*y + m[5]);

    return 2;
}

/* ↓ :inverse() — returns a new matrix, does not mutate ↓ */
static int matInverse(lua_State *L) {
    float m[9];
    matGet(L, 1, m);

    float det = m[0]*m[4] - m[1]*m[3];
    float inv[9];

    if (fabsf(det) < 1e-9f) {
        vanir_log("Matrix:inverse: singular matrix, returning identity");
        mat3Identity(inv);
    } else {
        float invDet = 1.0f / det;

        inv[0] =  m[4] * invDet;
        inv[1] = -m[1] * invDet;
        inv[3] = -m[3] * invDet;
        inv[4] =  m[0] * invDet;
        inv[2] = -(inv[0]*m[2] + inv[1]*m[5]);
        inv[5] = -(inv[3]*m[2] + inv[4]*m[5]);
        inv[6] = 0; inv[7] = 0; inv[8] = 1;
    }

    matPush(L, inv);
    return 1;
}

/* ↓ :copy() — returns a new matrix ↓ */
static int matCopy(lua_State *L) {
    float m[9];
    matGet(L, 1, m);
    matPush(L, m);
    return 1;
}

static const luaL_Reg matMethods[] = {
    {"translate",       matTranslate},
    {"rotate",          matRotate},
    {"scale",           matScale},
    {"transformPoint",  matTransformPoint},
    {"inverse",         matInverse},
    {"copy",            matCopy},

    {NULL, NULL}
};

static const luaL_Reg matMeta[] = {
    {"__tostring", matToString},
    {"__mul",      matMul},
    {"__eq",       matEq},

    {NULL, NULL}
};

/* ↓ Matrix(angle_or_nil, vector_or_nil)                                        ↓ */
/* ↓ no args → identity                                                         ↓ */
/* ↓ first arg = number → rotation by that many degrees                         ↓ */
/* ↓ first arg = Angle  → rotation by .yaw                                      ↓ */
/* ↓ second arg = Vector → translation by .x/.y                                 ↓ */
int Matrix(lua_State *L) {
    float m[9];
    mat3Identity(m);

    if (!lua_isnoneornil(L, 1)) {
        float deg;

        if (lua_isnumber(L, 1)) {
            deg = (float)lua_tonumber(L, 1);
        } else {
            lua_getfield(L, 1, "yaw");
            deg = (float)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }

        float rad = deg * (float)(M_PI / 180.0);
        float c   = cosf(rad);
        float s   = sinf(rad);

        m[0] = c; m[1] = -s;
        m[3] = s; m[4] =  c;

    }

    if (!lua_isnoneornil(L, 2)) {
        lua_getfield(L, 2, "x");
        lua_getfield(L, 2, "y");

        m[2] = (float)lua_tonumber(L, -2);
        m[5] = (float)lua_tonumber(L, -1);

        lua_pop(L, 2);

    }

    matPush(L, m);
    return 1;
}

/* ── public helpers used by render.c ─────────────────────────────────────── */

void matrixGet(lua_State *L, int idx, float m[9]) {
    matGet(L, idx, m);
}

void matrixPush(lua_State *L, const float m[9]) {
    matPush(L, m);
}

void matrixIdentity(float m[9]) {
    mat3Identity(m);
}

void matrixTransformPoint(const float m[9], float x, float y, float *ox, float *oy) {
    *ox = m[0]*x + m[1]*y + m[2];
    *oy = m[3]*x + m[4]*y + m[5];
}
