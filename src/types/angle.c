#include "common.h"

#include <math.h>

static const luaL_Reg angleMethods[];
static const luaL_Reg angleMeta[];

struct VanirAng *pushAngle(lua_State *L, float p, float y, float r) {
    struct VanirAng *a = (struct VanirAng *)lua_newuserdata(L, sizeof(struct VanirAng));
   
    a->p = p;
    a->y = y;
    a->r = r;

    addMethodsUD(L, "vanir.Angle", angleMethods, angleMeta);

    return a;
}

/* ↓ __tostring ↓ */
static int toStringAngle(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);

    lua_pushfstring(L, "(%f, %f, %f)", a->p, a->y, a->r);

    return 1;
}

/* ↓ __mul: Angle * scalar  or  scalar * Angle ↓ */
static int angleMul(lua_State *L) {
    float p, y, r, s;

    if (lua_isnumber(L, 1)) {
        s = (float)lua_tonumber(L, 1);
        struct VanirAng *a = checkAng(L, 2);
        p = a->p; y = a->y; r = a->r;
    } else {
        struct VanirAng *a = checkAng(L, 1);
        p = a->p; y = a->y; r = a->r;
        s = (float)luaL_checknumber(L, 2);
    }

    pushAngle(L, p * s, y * s, r * s);

    return 1;
}

/* ↓ __div: Angle / scalar ↓ */
static int angleDiv(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);
    float s = (float)luaL_checknumber(L, 2);

    pushAngle(L, a->p / s, a->y / s, a->r / s);

    return 1;
}

/* ↓ __unm: -Angle ↓ */
static int angleUnm(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);

    pushAngle(L, -a->p, -a->y, -a->r);

    return 1;
}

/* ↓ __eq ↓ */
static int angleEq(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);
    struct VanirAng *b = checkAng(L, 2);

    lua_pushboolean(L, a->p == b->p && a->y == b->y && a->r == b->r);

    return 1;
}

/* ↓ __add ↓ */
static int angleAdd(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);
    struct VanirAng *b = checkAng(L, 2);

    pushAngle(L, a->p + b->p, a->y + b->y, a->r + b->r);

    return 1;
}

/* ↓ __sub ↓ */
static int angleSub(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);
    struct VanirAng *b = checkAng(L, 2);

    pushAngle(L, a->p - b->p, a->y - b->y, a->r - b->r);

    return 1;
}

/* ↓ :isZero() ↓ */
static int angleIsZero(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);

    lua_pushboolean(L, a->p == 0.0f && a->y == 0.0f && a->r == 0.0f);

    return 1;
}

/* ↓ :getForward() → Vector ↓ */
static int angleGetForward(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);
    float pitch = degToRad(a->p);
    float yaw   = degToRad(a->y);

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw),   sy = sinf(yaw);

    struct VanirVec *v = (struct VanirVec *)lua_newuserdata(L, sizeof(struct VanirVec));
    
    v->x = cp * cy;
    v->y = cp * sy;
    v->z = -sp;

    luaL_setmetatable(L, "vanir.Vector");

    return 1;
}

/* ↓ :getRight() → Vector ↓ */
static int angleGetRight(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);
    float pitch = degToRad(a->p);
    float yaw   = degToRad(a->y);
    float roll  = degToRad(a->r);

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw),   sy = sinf(yaw);
    float cr = cosf(roll),  sr = sinf(roll);

    struct VanirVec *v = (struct VanirVec *)lua_newuserdata(L, sizeof(struct VanirVec));

    v->x = cr*sy - sr*sp*cy;
    v->y = -cr*cy - sr*sp*sy;
    v->z = -sr*cp;

    luaL_setmetatable(L, "vanir.Vector");

    return 1;
}

/* ↓ :getUp() → Vector ↓ */
static int angleGetUp(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);
    float pitch = degToRad(a->p);
    float yaw   = degToRad(a->y);
    float roll  = degToRad(a->r);

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw),   sy = sinf(yaw);
    float cr = cosf(roll),  sr = sinf(roll);

    struct VanirVec *v = (struct VanirVec *)lua_newuserdata(L, sizeof(struct VanirVec));

    v->x = -sr*sy + cr*sp*cy;
    v->y =  sr*cy + cr*sp*sy;
    v->z =  cr*cp;

    luaL_setmetatable(L, "vanir.Vector");

    return 1;
}

/* ↓ :rotateAroundAxis(axis_vec, degrees) → new Angle ↓ */
static int angleRotateAroundAxis(lua_State *L) {
    struct VanirAng *ang = checkAng(L, 1);
    struct VanirVec *axis = checkVec(L, 2);
    float deg = (float)luaL_checknumber(L, 3);
    float rad = degToRad(deg);

    float pitch = degToRad(ang->p);
    float yaw   = degToRad(ang->y);

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw),   sy = sinf(yaw);

    float fx = cp * cy, fy = cp * sy, fz = -sp;

    float c = cosf(rad), s = sinf(rad);
    float dot = axis->x*fx + axis->y*fy + axis->z*fz;

    float rx = fx*c + (axis->y*fz - axis->z*fy)*s + axis->x*dot*(1.0f - c);
    float ry = fy*c + (axis->z*fx - axis->x*fz)*s + axis->y*dot*(1.0f - c);
    float rz = fz*c + (axis->x*fy - axis->y*fx)*s + axis->z*dot*(1.0f - c);

    float newPitch = radToDeg(-asinf(rz));
    float newYaw = radToDeg(atan2f(ry, rx));

    pushAngle(L, newPitch, newYaw, ang->r);

    return 1;
}

/* ↓ :round([decimals]) → new Angle ↓ */
static int angleRound(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);
    float mul = 1.0f;

    if (!lua_isnoneornil(L, 2)) {
        int dec = (int)lua_tointeger(L, 2);

        for (int i = 0; i < dec; i++)
            mul *= 10.0f;
    }

    pushAngle(L,
        roundf(a->p * mul) / mul,
        roundf(a->y * mul) / mul,
        roundf(a->r * mul) / mul
    );

    return 1;
}

/* ↓ :clone() ↓ */
static int angleClone(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);

    pushAngle(L, a->p, a->y, a->r);

    return 1;
}

/* ↓ :set(p, y, r) — mutates in-place, returns self ↓ */
static int angleSet(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);

    a->p = (float)luaL_checknumber(L, 2);
    a->y = (float)luaL_checknumber(L, 3);
    a->r = (float)luaL_checknumber(L, 4);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setZero() — mutates in-place, returns self ↓ */
static int angleSetZero(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);

    a->p = 0.0f;
    a->y = 0.0f;
    a->r = 0.0f;

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setP(v) ↓ */
static int angleSetP(lua_State *L) {
    checkAng(L, 1)->p = (float)luaL_checknumber(L, 2);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setY(v) ↓ */
static int angleSetY(lua_State *L) {
    checkAng(L, 1)->y = (float)luaL_checknumber(L, 2);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setR(v) ↓ */
static int angleSetR(lua_State *L) {
    checkAng(L, 1)->r = (float)luaL_checknumber(L, 2);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ __index — expose p/y/r fields from userdata to Lua                    ↓ */
/* ↓ also exposes pitch/yaw/roll aliases (used by Quaternion:toAngle etc.) ↓ */
static int angleIndex(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);
    const char *key = luaL_checkstring(L, 2);

    if (key[1] == '\0') {
        if (key[0] == 'p') { lua_pushnumber(L, a->p); return 1; }
        if (key[0] == 'y') { lua_pushnumber(L, a->y); return 1; }
        if (key[0] == 'r') { lua_pushnumber(L, a->r); return 1; }
    }

    /* ↓ long-name aliases ↓ */
    if (strcmp(key, "pitch") == 0) { lua_pushnumber(L, a->p); return 1; }
    if (strcmp(key, "yaw") == 0) { lua_pushnumber(L, a->y); return 1; }
    if (strcmp(key, "roll") == 0) { lua_pushnumber(L, a->r); return 1; }

    /* ↓ fall through to method table ↓ */
    vanirUD_indexFallback(L, "vanir.Angle", key);

    return 1;
}

/* ↓ __newindex — allow a.p = n and a.pitch = n style assignment ↓ */
static int angleNewIndex(lua_State *L) {
    struct VanirAng *a = checkAng(L, 1);
    const char *key = luaL_checkstring(L, 2);
    float val = (float)luaL_checknumber(L, 3);

    if (key[1] == '\0') {
        if (key[0] == 'p') { a->p = val; return 0; }
        if (key[0] == 'y') { a->y = val; return 0; }
        if (key[0] == 'r') { a->r = val; return 0; }
    }

    /* ↓ long-name aliases ↓ */
    if (strcmp(key, "pitch") == 0) { a->p = val; return 0; }
    if (strcmp(key, "yaw")   == 0) { a->y = val; return 0; }
    if (strcmp(key, "roll")  == 0) { a->r = val; return 0; }

    return luaL_error(L, "Angle has no field '%s'", key);
}

static const luaL_Reg angleMethods[] = {
    {"isZero",           angleIsZero},
    {"getForward",       angleGetForward},
    {"getRight",         angleGetRight},
    {"getUp",            angleGetUp},
    {"rotateAroundAxis", angleRotateAroundAxis},
    {"round",            angleRound},
    {"clone",            angleClone},
    {"set",              angleSet},
    {"setZero",          angleSetZero},
    {"setP",             angleSetP},
    {"setY",             angleSetY},
    {"setR",             angleSetR},

    {NULL, NULL}
};

static const luaL_Reg angleMeta[] = {
    {"__tostring", toStringAngle},
    {"__mul",      angleMul},
    {"__div",      angleDiv},
    {"__unm",      angleUnm},
    {"__eq",       angleEq},
    {"__add",      angleAdd},
    {"__sub",      angleSub},
    {"__index",    angleIndex},
    {"__newindex", angleNewIndex},

    {NULL, NULL}
};

/* ↓ Angle(p, y, r) ↓ */
int Angle(lua_State *L) {
    float p = (float)luaL_optnumber(L, 1, 0.0);
    float y = (float)luaL_optnumber(L, 2, 0.0);
    float r = (float)luaL_optnumber(L, 3, 0.0);

    pushAngle(L, p, y, r);

    return 1;
}