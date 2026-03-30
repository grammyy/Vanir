/* quaternion.c */
#include "common.h"

#include <math.h>

static const luaL_Reg quatMethods[];
static const luaL_Reg quatMeta[];

static void pushQuat(lua_State *L, float x, float y, float z, float w) {
    lua_newtable(L);
    setFieldNumber(L, "x", x);
    setFieldNumber(L, "y", y);
    setFieldNumber(L, "z", z);
    setFieldNumber(L, "w", w);
    addMethods(L, "vanir.Quaternion", quatMethods, quatMeta);
}

static int toStringQuat(lua_State *L) {
    float x = getfieldf(L, 1, "x");
    float y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z");
    float w = getfieldf(L, 1, "w");

    lua_pushfstring(L, "(%f, %f, %f, %f)", x, y, z, w);

    return 1;
}

/* ↓ quat * quat — Hamilton product ↓ */
static int quatMul(lua_State *L) {
    float ax = getfieldf(L, 1, "x"), ay = getfieldf(L, 1, "y");
    float az = getfieldf(L, 1, "z"), aw = getfieldf(L, 1, "w");
    float bx = getfieldf(L, 2, "x"), by = getfieldf(L, 2, "y");
    float bz = getfieldf(L, 2, "z"), bw = getfieldf(L, 2, "w");

    pushQuat(L,
        aw*bx + ax*bw + ay*bz - az*by,
        aw*by - ax*bz + ay*bw + az*bx,
        aw*bz + ax*by - ay*bx + az*bw,
        aw*bw - ax*bx - ay*by - az*bz
    );

    return 1;
}

static int quatEq(lua_State *L) {
    lua_pushboolean(L,
        getfieldf(L, 1, "x") == getfieldf(L, 2, "x") &&
        getfieldf(L, 1, "y") == getfieldf(L, 2, "y") &&
        getfieldf(L, 1, "z") == getfieldf(L, 2, "z") &&
        getfieldf(L, 1, "w") == getfieldf(L, 2, "w")
    );

    return 1;
}

/* ↓ quat:length() — magnitude ↓ */
static int quatLength(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");

    lua_pushnumber(L, sqrtf(x*x + y*y + z*z + w*w));

    return 1;
}

/* ↓ quat:normalize() ↓ */
static int quatNormalize(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");
    float len = sqrtf(x*x + y*y + z*z + w*w);

    if (len > 0.0f) {
        pushQuat(L, x/len, y/len, z/len, w/len);
    } else {
        pushQuat(L, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    return 1;
}

/* ↓ quat:conjugate() — same as inverse for unit quaternions ↓ */
static int quatConjugate(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");

    pushQuat(L, -x, -y, -z, w);

    return 1;
}

/* ↓ quat:dot(other) ↓ */
static int quatDot(lua_State *L) {
    float ax = getfieldf(L, 1, "x"), ay = getfieldf(L, 1, "y");
    float az = getfieldf(L, 1, "z"), aw = getfieldf(L, 1, "w");
    float bx = getfieldf(L, 2, "x"), by = getfieldf(L, 2, "y");
    float bz = getfieldf(L, 2, "z"), bw = getfieldf(L, 2, "w");

    lua_pushnumber(L, ax*bx + ay*by + az*bz + aw*bw);

    return 1;
}

/* ↓ quat:slerp(other, t) — spherical linear interpolation ↓ */
static int quatSlerp(lua_State *L) {
    float ax = getfieldf(L, 1, "x"), ay = getfieldf(L, 1, "y");
    float az = getfieldf(L, 1, "z"), aw = getfieldf(L, 1, "w");
    float bx = getfieldf(L, 2, "x"), by = getfieldf(L, 2, "y");
    float bz = getfieldf(L, 2, "z"), bw = getfieldf(L, 2, "w");
    float t = (float)luaL_checknumber(L, 3);

    float dot = ax*bx + ay*by + az*bz + aw*bw;

    /* ↓ flip second quat if dot is negative to take the short arc ↓ */
    if (dot < 0.0f) {
        bx=-bx;
        by=-by;
        bz=-bz;
        bw=-bw;
        dot=-dot;
    }

    if (dot > 0.9995f) {
        float rx = ax + t*(bx-ax), ry = ay + t*(by-ay);
        float rz = az + t*(bz-az), rw = aw + t*(bw-aw);
        float len = sqrtf(rx*rx + ry*ry + rz*rz + rw*rw);
        /* ↑ quaternions are nearly identical; lerp and normalize ↑ */

        pushQuat(L, rx/len, ry/len, rz/len, rw/len);

        return 1;
    }

    float theta0 = acosf(dot);
    float theta  = theta0 * t;
    float sinT   = sinf(theta);
    float sinT0  = sinf(theta0);
    float s0     = cosf(theta) - dot * sinT / sinT0;
    float s1     = sinT / sinT0;

    pushQuat(L,
        s0*ax + s1*bx,
        s0*ay + s1*by,
        s0*az + s1*bz,
        s0*aw + s1*bw
    );

    return 1;
}

/* ↓ quat:toAngle() — converts to Angle (roll/pitch/yaw) in degrees ↓ */
static int quatToAngle(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");

    /* ↓ ZXY order: yaw, roll, pitch ↓ */
    float sinR = 2.0f*(w*x + y*z),  cosR = 1.0f - 2.0f*(x*x + y*y);
    float sinP = 2.0f*(w*y - z*x);
    float sinY = 2.0f*(w*z + x*y),  cosY = 1.0f - 2.0f*(y*y + z*z);

    float roll  = atan2f(sinR, cosR) * (180.0f / 3.14159265f);
    float pitch = (fabsf(sinP) >= 1.0f) ? copysignf(90.0f, sinP) : asinf(sinP) * (180.0f / 3.14159265f);
    float yaw   = atan2f(sinY, cosY) * (180.0f / 3.14159265f);

    lua_newtable(L);
    setFieldNumber(L, "roll",  roll);
    setFieldNumber(L, "pitch", pitch);
    setFieldNumber(L, "yaw",   yaw);
    addMethods(L, "vanir.Angle", NULL, NULL);

    return 1;
}

static const luaL_Reg quatMethods[] = {
    {"length",    quatLength},
    {"normalize", quatNormalize},
    {"conjugate", quatConjugate},
    {"dot",       quatDot},
    {"slerp",     quatSlerp},
    {"toAngle",   quatToAngle},

    {NULL, NULL}
};

static const luaL_Reg quatMeta[] = {
    {"__tostring", toStringQuat},
    {"__mul",      quatMul},
    {"__eq",       quatEq},

    {NULL, NULL}
};

int Quaternion(lua_State *L) {
    float x = (float)luaL_optnumber(L, 1, 0.0);
    float y = (float)luaL_optnumber(L, 2, 0.0);
    float z = (float)luaL_optnumber(L, 3, 0.0);
    float w = (float)luaL_optnumber(L, 4, 1.0);

    pushQuat(L, x, y, z, w);

    return 1;
}