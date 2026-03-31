#include "common.h"

#include <math.h>

static const luaL_Reg angleMethods[];
static const luaL_Reg angleMeta[];

static void pushAngle(lua_State *L, float p, float y, float r) {
    lua_newtable(L);
    setFieldNumber(L, "p", p);
    setFieldNumber(L, "y", y);
    setFieldNumber(L, "r", r);
    addMethods(L, "vanir.Angle", angleMethods, angleMeta);
}

/* ↓ __tostring ↓ */
static int toStringAngle(lua_State *L) {
    float p = getfieldf(L, 1, "p");
    float y = getfieldf(L, 1, "y");
    float r = getfieldf(L, 1, "r");

    lua_pushfstring(L, "(%f, %f, %f)", p, y, r);

    return 1;
}

/* ↓ __mul: Angle * scalar  or  scalar * Angle ↓ */
static int angleMul(lua_State *L) {
    float p, y, r, s;

    if (lua_isnumber(L, 1)) {
        s = (float)lua_tonumber(L, 1);
        p = getfieldf(L, 2, "p"); y = getfieldf(L, 2, "y"); r = getfieldf(L, 2, "r");
    } else {
        p = getfieldf(L, 1, "p"); y = getfieldf(L, 1, "y"); r = getfieldf(L, 1, "r");
        s = (float)luaL_checknumber(L, 2);
    }

    pushAngle(L, p * s, y * s, r * s);

    return 1;
}

/* ↓ __div: Angle / scalar ↓ */
static int angleDiv(lua_State *L) {
    float p = getfieldf(L, 1, "p");
    float y = getfieldf(L, 1, "y");
    float r = getfieldf(L, 1, "r");
    float s = (float)luaL_checknumber(L, 2);

    pushAngle(L, p / s, y / s, r / s);

    return 1;
}

/* ↓ __unm: -Angle ↓ */
static int angleUnm(lua_State *L) {
    pushAngle(L, -getfieldf(L, 1, "p"), -getfieldf(L, 1, "y"), -getfieldf(L, 1, "r"));

    return 1;
}

/* ↓ __eq ↓ */
static int angleEq(lua_State *L) {
    lua_pushboolean(L,
        getfieldf(L, 1, "p") == getfieldf(L, 2, "p") &&
        getfieldf(L, 1, "y") == getfieldf(L, 2, "y") &&
        getfieldf(L, 1, "r") == getfieldf(L, 2, "r")
    );

    return 1;
}

/* ↓ __add ↓ */
static int angleAdd(lua_State *L) {
    pushAngle(L,
        getfieldf(L, 1, "p") + getfieldf(L, 2, "p"),
        getfieldf(L, 1, "y") + getfieldf(L, 2, "y"),
        getfieldf(L, 1, "r") + getfieldf(L, 2, "r")
    );

    return 1;
}

/* ↓ __sub ↓ */
static int angleSub(lua_State *L) {
    pushAngle(L,
        getfieldf(L, 1, "p") - getfieldf(L, 2, "p"),
        getfieldf(L, 1, "y") - getfieldf(L, 2, "y"),
        getfieldf(L, 1, "r") - getfieldf(L, 2, "r")
    );

    return 1;
}

/* ↓ :isZero() ↓ */
static int angleIsZero(lua_State *L) {
    lua_pushboolean(L,
        getfieldf(L, 1, "p") == 0.0f &&
        getfieldf(L, 1, "y") == 0.0f &&
        getfieldf(L, 1, "r") == 0.0f
    );

    return 1;
}

/* ↓ :getForward() → Vector ↓ */
static int angleGetForward(lua_State *L) {
    float pitch = degToRad(getfieldf(L, 1, "p"));
    float yaw   = degToRad(getfieldf(L, 1, "y"));

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw),   sy = sinf(yaw);

    lua_newtable(L);
    setFieldNumber(L, "x", cp * cy);
    setFieldNumber(L, "y", cp * sy);
    setFieldNumber(L, "z", -sp);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getRight() → Vector ↓ */
static int angleGetRight(lua_State *L) {
    float pitch = degToRad(getfieldf(L, 1, "p"));
    float yaw = degToRad(getfieldf(L, 1, "y"));
    float roll = degToRad(getfieldf(L, 1, "r"));

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw), sy = sinf(yaw);
    float cr = cosf(roll), sr = sinf(roll);

    lua_newtable(L);
    setFieldNumber(L, "x", cr*sy - sr*sp*cy);
    setFieldNumber(L, "y", -cr*cy - sr*sp*sy);
    setFieldNumber(L, "z", -sr*cp);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getUp() → Vector ↓ */
static int angleGetUp(lua_State *L) {
    float pitch = degToRad(getfieldf(L, 1, "p"));
    float yaw   = degToRad(getfieldf(L, 1, "y"));
    float roll  = degToRad(getfieldf(L, 1, "r"));

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw),   sy = sinf(yaw);
    float cr = cosf(roll),  sr = sinf(roll);

    lua_newtable(L);
    setFieldNumber(L, "x", -sr*sy + cr*sp*cy);
    setFieldNumber(L, "y",  sr*cy + cr*sp*sy);
    setFieldNumber(L, "z",  cr*cp);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :rotateAroundAxis(axis_vec, degrees) → new Angle ↓ */
static int angleRotateAroundAxis(lua_State *L) {
    float ax = getfieldf(L, 2, "x");
    float ay = getfieldf(L, 2, "y");
    float az = getfieldf(L, 2, "z");
    float deg = (float)luaL_checknumber(L, 3);
    float rad = degToRad(deg);

    float pitch = degToRad(getfieldf(L, 1, "p"));
    float yaw   = degToRad(getfieldf(L, 1, "y"));

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw), sy = sinf(yaw);

    float fx = cp * cy, fy = cp * sy, fz = -sp;

    float c = cosf(rad), s = sinf(rad);
    float dot = ax*fx + ay*fy + az*fz;

    float rx = fx*c + (ay*fz - az*fy)*s + ax*dot*(1.0f - c);
    float ry = fy*c + (az*fx - ax*fz)*s + ay*dot*(1.0f - c);
    float rz = fz*c + (ax*fy - ay*fx)*s + az*dot*(1.0f - c);

    float newPitch = radToDeg(-asinf(rz));
    float newYaw = radToDeg(atan2f(ry, rx));

    pushAngle(L, newPitch, newYaw, getfieldf(L, 1, "r"));

    return 1;
}

/* ↓ :round([decimals]) → new Angle ↓ */
static int angleRound(lua_State *L) {
    float p = getfieldf(L, 1, "p");
    float y = getfieldf(L, 1, "y");
    float r = getfieldf(L, 1, "r");
    float mul = 1.0f;

    if (!lua_isnoneornil(L, 2)) {
        int dec = (int)lua_tointeger(L, 2);
        
        for (int i = 0; i < dec; i++) 
            mul *= 10.0f;
    }

    pushAngle(L,
        roundf(p * mul) / mul,
        roundf(y * mul) / mul,
        roundf(r * mul) / mul
    );

    return 1;
}

/* ↓ :clone() ↓ */
static int angleClone(lua_State *L) {
    pushAngle(L, getfieldf(L, 1, "p"), getfieldf(L, 1, "y"), getfieldf(L, 1, "r"));

    return 1;
}

/* ↓ :set(p, y, r) — mutates in-place, returns self ↓ */
static int angleSet(lua_State *L) {
    setFieldNumber(L, "p", (float)luaL_checknumber(L, 2));
    setFieldNumber(L, "y", (float)luaL_checknumber(L, 3));
    setFieldNumber(L, "r", (float)luaL_checknumber(L, 4));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setZero() — mutates in-place, returns self ↓ */
static int angleSetZero(lua_State *L) {
    setFieldNumber(L, "p", 0.0f);
    setFieldNumber(L, "y", 0.0f);
    setFieldNumber(L, "r", 0.0f);
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setP(v) ↓ */
static int angleSetP(lua_State *L) {
    setFieldNumber(L, "p", (float)luaL_checknumber(L, 2));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setY(v) ↓ */
static int angleSetY(lua_State *L) {
    setFieldNumber(L, "y", (float)luaL_checknumber(L, 2));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setR(v) ↓ */
static int angleSetR(lua_State *L) {
    setFieldNumber(L, "r", (float)luaL_checknumber(L, 2));
    lua_pushvalue(L, 1);

    return 1;
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