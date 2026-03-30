#include "common.h"

#include <math.h>

static const luaL_Reg vecMethods[];
static const luaL_Reg vecMeta[];

static int toStringVec(lua_State *L) {
    float x = getfieldf(L, 1, "x");
    float y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z");

    lua_pushfstring(L, "(%f, %f, %f)", x, y, z);

    return 1;
}

static int vecAdd(lua_State *L) {
    float ax = getfieldf(L, 1, "x"), ay = getfieldf(L, 1, "y"), az = getfieldf(L, 1, "z");
    float bx = getfieldf(L, 2, "x"), by = getfieldf(L, 2, "y"), bz = getfieldf(L, 2, "z");

    lua_newtable(L);
    setFieldNumber(L, "x", ax + bx);
    setFieldNumber(L, "y", ay + by);
    setFieldNumber(L, "z", az + bz);
    addMethods(L, "vanir.Vector", NULL, vecMeta);

    return 1;
}

static int vecSub(lua_State *L) {
    float ax = getfieldf(L, 1, "x"), ay = getfieldf(L, 1, "y"), az = getfieldf(L, 1, "z");
    float bx = getfieldf(L, 2, "x"), by = getfieldf(L, 2, "y"), bz = getfieldf(L, 2, "z");

    lua_newtable(L);
    setFieldNumber(L, "x", ax - bx);
    setFieldNumber(L, "y", ay - by);
    setFieldNumber(L, "z", az - bz);
    addMethods(L, "vanir.Vector", NULL, vecMeta);

    return 1;
}

/* ↓ scalar multiply: vec * number  or  number * vec ↓ */
static int vecMul(lua_State *L) {
    float x, y, z, s;
    
    if (lua_isnumber(L, 1)) {
        s = (float)lua_tonumber(L, 1);
        x = getfieldf(L, 2, "x"); y = getfieldf(L, 2, "y"); z = getfieldf(L, 2, "z");
    } else {
        x = getfieldf(L, 1, "x"); y = getfieldf(L, 1, "y"); z = getfieldf(L, 1, "z");
        s = (float)luaL_checknumber(L, 2);
    }

    lua_newtable(L);
    setFieldNumber(L, "x", x * s);
    setFieldNumber(L, "y", y * s);
    setFieldNumber(L, "z", z * s);
    addMethods(L, "vanir.Vector", NULL, vecMeta);

    return 1;
}

static int vecUnm(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");

    lua_newtable(L);
    setFieldNumber(L, "x", -x);
    setFieldNumber(L, "y", -y);
    setFieldNumber(L, "z", -z);
    addMethods(L, "vanir.Vector", NULL, vecMeta);

    return 1;
}

static int vecEq(lua_State *L) {
    lua_pushboolean(L,
        getfieldf(L, 1, "x") == getfieldf(L, 2, "x") &&
        getfieldf(L, 1, "y") == getfieldf(L, 2, "y") &&
        getfieldf(L, 1, "z") == getfieldf(L, 2, "z"));

    return 1;
}

/* #vec → magnitude */
static int vecLen(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");

    lua_pushnumber(L, sqrtf(x*x + y*y + z*z));

    return 1;
}

static int vecDot(lua_State *L) {
    float ax = getfieldf(L, 1, "x"), ay = getfieldf(L, 1, "y"), az = getfieldf(L, 1, "z");
    float bx = getfieldf(L, 2, "x"), by = getfieldf(L, 2, "y"), bz = getfieldf(L, 2, "z");

    lua_pushnumber(L, ax*bx + ay*by + az*bz);

    return 1;
}

static int vecCross(lua_State *L) {
    float ax = getfieldf(L, 1, "x"), ay = getfieldf(L, 1, "y"), az = getfieldf(L, 1, "z");
    float bx = getfieldf(L, 2, "x"), by = getfieldf(L, 2, "y"), bz = getfieldf(L, 2, "z");

    lua_newtable(L);
    setFieldNumber(L, "x", ay*bz - az*by);
    setFieldNumber(L, "y", az*bx - ax*bz);
    setFieldNumber(L, "z", ax*by - ay*bx);
    addMethods(L, "vanir.Vector", NULL, vecMeta);

    return 1;
}

static int vecNormalize(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");
    float len = sqrtf(x*x + y*y + z*z);

    lua_newtable(L);

    if (len > 0.0f) {
        setFieldNumber(L, "x", x / len);
        setFieldNumber(L, "y", y / len);
        setFieldNumber(L, "z", z / len);
    } else {
        setFieldNumber(L, "x", 0.0f);
        setFieldNumber(L, "y", 0.0f);
        setFieldNumber(L, "z", 0.0f);
    }

    addMethods(L, "vanir.Vector", NULL, vecMeta);

    return 1;
}

static int vecLength(lua_State *L) {
    return vecLen(L);
}

static const luaL_Reg vecMethods[] = {
    {"dot",       vecDot},
    {"cross",     vecCross},
    {"normalize", vecNormalize},
    {"length",    vecLength},

    {NULL, NULL}
};

static const luaL_Reg vecMeta[] = {
    {"__tostring", toStringVec},
    {"__add",      vecAdd},
    {"__sub",      vecSub},
    {"__mul",      vecMul},
    {"__unm",      vecUnm},
    {"__eq",       vecEq},
    {"__len",      vecLen},

    {NULL, NULL}
};

int Vector(lua_State *L) {
    float x = (float)luaL_optnumber(L, 1, 0.0);
    float y = (float)luaL_optnumber(L, 2, 0.0);
    float z = (float)luaL_optnumber(L, 3, 0.0);

    lua_newtable(L);
    setFieldNumber(L, "x", x);
    setFieldNumber(L, "y", y);
    setFieldNumber(L, "z", z);

    addMethods(L, "vanir.Vector", vecMethods, vecMeta);

    return 1;
}