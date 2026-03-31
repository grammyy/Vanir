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

/* ↓ __tostring ↓ */
static int toStringQuat(lua_State *L) {
    float x = getfieldf(L, 1, "x");
    float y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z");
    float w = getfieldf(L, 1, "w");

    lua_pushfstring(L, "(%f, %f, %f, %f)", x, y, z, w);

    return 1;
}

/* ↓ __add ↓ */
static int quatAdd(lua_State *L) {
    pushQuat(L,
        getfieldf(L, 1, "x") + getfieldf(L, 2, "x"),
        getfieldf(L, 1, "y") + getfieldf(L, 2, "y"),
        getfieldf(L, 1, "z") + getfieldf(L, 2, "z"),
        getfieldf(L, 1, "w") + getfieldf(L, 2, "w")
    );

    return 1;
}

/* ↓ __sub ↓ */
static int quatSub(lua_State *L) {
    pushQuat(L,
        getfieldf(L, 1, "x") - getfieldf(L, 2, "x"),
        getfieldf(L, 1, "y") - getfieldf(L, 2, "y"),
        getfieldf(L, 1, "z") - getfieldf(L, 2, "z"),
        getfieldf(L, 1, "w") - getfieldf(L, 2, "w")
    );

    return 1;
}

/* ↓ __mul: quat * quat — Hamilton product  OR  quat * scalar ↓ */
static int quatMul(lua_State *L) {
    if (lua_isnumber(L, 2)) {
        float s = (float)lua_tonumber(L, 2);

        pushQuat(L,
            getfieldf(L, 1, "x") * s,
            getfieldf(L, 1, "y") * s,
            getfieldf(L, 1, "z") * s,
            getfieldf(L, 1, "w") * s
        );

        return 1;
    }

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

/* ↓ __div: quat / scalar ↓ */
static int quatDiv(lua_State *L) {
    float s = (float)luaL_checknumber(L, 2);

    pushQuat(L,
        getfieldf(L, 1, "x") / s,
        getfieldf(L, 1, "y") / s,
        getfieldf(L, 1, "z") / s,
        getfieldf(L, 1, "w") / s
    );

    return 1;
}

/* ↓ __unm ↓ */
static int quatUnm(lua_State *L) {
    pushQuat(L,
        -getfieldf(L, 1, "x"),
        -getfieldf(L, 1, "y"),
        -getfieldf(L, 1, "z"),
        -getfieldf(L, 1, "w")
    );

    return 1;
}

/* ↓ __eq ↓ */
static int quatEq(lua_State *L) {
    lua_pushboolean(L,
        getfieldf(L, 1, "x") == getfieldf(L, 2, "x") &&
        getfieldf(L, 1, "y") == getfieldf(L, 2, "y") &&
        getfieldf(L, 1, "z") == getfieldf(L, 2, "z") &&
        getfieldf(L, 1, "w") == getfieldf(L, 2, "w")
    );

    return 1;
}

/* ↓ :set(x, y, z, w) — mutates in-place, returns self ↓ */
static int quatSet(lua_State *L) {
    setFieldNumber(L, "x", (float)luaL_checknumber(L, 2));
    setFieldNumber(L, "y", (float)luaL_checknumber(L, 3));
    setFieldNumber(L, "z", (float)luaL_checknumber(L, 4));
    setFieldNumber(L, "w", (float)luaL_checknumber(L, 5));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setIdentity() — mutates in-place, returns self ↓ */
static int quatSetIdentity(lua_State *L) {
    setFieldNumber(L, "x", 0.0f);
    setFieldNumber(L, "y", 0.0f);
    setFieldNumber(L, "z", 0.0f);
    setFieldNumber(L, "w", 1.0f);
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :isIdentity() ↓ */
static int quatIsIdentity(lua_State *L) {
    lua_pushboolean(L,
        getfieldf(L, 1, "x") == 0.0f &&
        getfieldf(L, 1, "y") == 0.0f &&
        getfieldf(L, 1, "z") == 0.0f &&
        getfieldf(L, 1, "w") == 1.0f
    );

    return 1;
}

/* ↓ :clone() ↓ */
static int quatClone(lua_State *L) {
    pushQuat(L, getfieldf(L, 1, "x"), getfieldf(L, 1, "y"), getfieldf(L, 1, "z"), getfieldf(L, 1, "w"));

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

    float roll = atan2f(sinR, cosR) * (180.0f / 3.14159265f);
    float pitch = (fabsf(sinP) >= 1.0f) ? copysignf(90.0f, sinP) : asinf(sinP) * (180.0f / 3.14159265f);
    float yaw  = atan2f(sinY, cosY) * (180.0f / 3.14159265f);

    lua_newtable(L);
    setFieldNumber(L, "roll",  roll);
    setFieldNumber(L, "pitch", pitch);
    setFieldNumber(L, "yaw",   yaw);

    luaL_getmetatable(L, "vanir.Angle");

    if (!lua_isnil(L, -1))
        lua_setmetatable(L, -2);
    else
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getAngle() → Angle (roll/pitch/yaw in degrees) ↓ */
static int quatGetAngle(lua_State *L) {
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

    luaL_getmetatable(L, "vanir.Angle");

    if (!lua_isnil(L, -1))
        lua_setmetatable(L, -2);
    else
        lua_pop(L, 1);

    return 1;
}

/* ↓ :setAngle(angle) — sets quat from Angle (pitch/yaw/roll in degrees), mutates in-place, returns self ↓ */
static int quatSetAngle(lua_State *L) {
    float pitch = degToRad(getfieldf(L, 2, "pitch")) * 0.5f;
    float yaw   = degToRad(getfieldf(L, 2, "yaw")) * 0.5f;
    float roll  = degToRad(getfieldf(L, 2, "roll")) * 0.5f;

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw),   sy = sinf(yaw);
    float cr = cosf(roll),  sr = sinf(roll);

    setFieldNumber(L, "x", sp*cy*cr + cp*sy*sr);
    setFieldNumber(L, "y", cp*sy*cr - sp*cy*sr);
    setFieldNumber(L, "z", cp*cy*sr - sp*sy*cr);
    setFieldNumber(L, "w", cp*cy*cr + sp*sy*sr);
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getMatrix() → Matrix (3x3 rotation matrix) ↓ */
static int quatGetMatrix(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");

    float x2 = x+x, y2 = y+y, z2 = z+z;
    float xx = x*x2, xy = x*y2, xz = x*z2;
    float yy = y*y2, yz = y*z2, zz = z*z2;
    float wx = w*x2, wy = w*y2, wz = w*z2;

    /* ↓ row-major 3x3, stored as array indices 1..9 ↓ */
    float m[9] = {
        1.0f-(yy+zz), xy-wz,        xz+wy,
        xy+wz,        1.0f-(xx+zz), yz-wx,
        xz-wy,        yz+wx,        1.0f-(xx+yy)
    };

    lua_createtable(L, 9, 0);

    for (int i = 0; i < 9; i++) {
        lua_pushnumber(L, m[i]);
        lua_rawseti(L, -2, i + 1);
    }

    luaL_getmetatable(L, "vanir.Matrix");

    if (!lua_isnil(L, -1))
        lua_setmetatable(L, -2);
    else
        lua_pop(L, 1);

    return 1;
}

/* ↓ :setMatrix(matrix) — sets quat from a 3x3 Matrix, mutates in-place, returns self ↓ */
static int quatSetMatrix(lua_State *L) {
    float m[9];

    for (int i = 0; i < 9; i++) {
        lua_rawgeti(L, 2, i + 1);
        m[i] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    /* ↓ Shepperd's method ↓ */
    float trace = m[0] + m[4] + m[8];
    float x, y, z, w;

    if (trace > 0.0f) {
        float s = 0.5f / sqrtf(trace + 1.0f);
        w = 0.25f / s;
        x = (m[7] - m[5]) * s;
        y = (m[2] - m[6]) * s;
        z = (m[3] - m[1]) * s;
    } else if (m[0] > m[4] && m[0] > m[8]) {
        float s = 2.0f * sqrtf(1.0f + m[0] - m[4] - m[8]);
        w = (m[7] - m[5]) / s;
        x = 0.25f * s;
        y = (m[1] + m[3]) / s;
        z = (m[2] + m[6]) / s;
    } else if (m[4] > m[8]) {
        float s = 2.0f * sqrtf(1.0f + m[4] - m[0] - m[8]);
        w = (m[2] - m[6]) / s;
        x = (m[1] + m[3]) / s;
        y = 0.25f * s;
        z = (m[5] + m[7]) / s;
    } else {
        float s = 2.0f * sqrtf(1.0f + m[8] - m[0] - m[4]);
        w = (m[3] - m[1]) / s;
        x = (m[2] + m[6]) / s;
        y = (m[5] + m[7]) / s;
        z = 0.25f * s;
    }

    setFieldNumber(L, "x", x);
    setFieldNumber(L, "y", y);
    setFieldNumber(L, "z", z);
    setFieldNumber(L, "w", w);
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setAxisAngle(axis_vec, degrees) — mutates in-place, returns self ↓ */
static int quatSetAxisAngle(lua_State *L) {
    float ax = getfieldf(L, 2, "x");
    float ay = getfieldf(L, 2, "y");
    float az = getfieldf(L, 2, "z");
    float half = degToRad((float)luaL_checknumber(L, 3)) * 0.5f;
    float s = sinf(half);

    setFieldNumber(L, "x", ax * s);
    setFieldNumber(L, "y", ay * s);
    setFieldNumber(L, "z", az * s);
    setFieldNumber(L, "w", cosf(half));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getAxisAngle() → axis_vec, degrees ↓ */
static int quatGetAxisAngle(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");

    float sinHalf = sqrtf(x*x + y*y + z*z);
    float degrees = 2.0f * atan2f(sinHalf, w) * (180.0f / 3.14159265f);
    float ax, ay, az;

    if (sinHalf > 0.0001f) {
        ax = x / sinHalf;
        ay = y / sinHalf;
        az = z / sinHalf;
    } else {
        /* ↓ identity — axis is arbitrary ↓ */
        ax = 0.0f;
        ay = 0.0f;
        az = 1.0f;
    }

    lua_newtable(L);
    setFieldNumber(L, "x", ax);
    setFieldNumber(L, "y", ay);
    setFieldNumber(L, "z", az);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1))
        lua_setmetatable(L, -2);
    else
        lua_pop(L, 1);

    lua_pushnumber(L, degrees);

    return 2;
}

/* ↓ :rotateAroundAxis(axis_vec, degrees) → new Quaternion ↓ */
static int quatRotateAroundAxis(lua_State *L) {
    float ax = getfieldf(L, 2, "x");
    float ay = getfieldf(L, 2, "y");
    float az = getfieldf(L, 2, "z");
    float half = degToRad((float)luaL_checknumber(L, 3)) * 0.5f;
    float s = sinf(half);

    float bx = ax * s, by = ay * s, bz = az * s, bw = cosf(half);

    float ax2 = getfieldf(L, 1, "x"), ay2 = getfieldf(L, 1, "y");
    float az2 = getfieldf(L, 1, "z"), aw  = getfieldf(L, 1, "w");

    pushQuat(L,
        aw*bx + ax2*bw + ay2*bz - az2*by,
        aw*by - ax2*bz + ay2*bw + az2*bx,
        aw*bz + ax2*by - ay2*bx + az2*bw,
        aw*bw - ax2*bx - ay2*by - az2*bz
    );

    return 1;
}

/* ↓ :dot(other) ↓ */
static int quatDot(lua_State *L) {
    lua_pushnumber(L,
        getfieldf(L, 1, "x") * getfieldf(L, 2, "x") +
        getfieldf(L, 1, "y") * getfieldf(L, 2, "y") +
        getfieldf(L, 1, "z") * getfieldf(L, 2, "z") +
        getfieldf(L, 1, "w") * getfieldf(L, 2, "w")
    );

    return 1;
}

/* ↓ :length() — magnitude ↓ */
static int quatLength(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");

    lua_pushnumber(L, sqrtf(x*x + y*y + z*z + w*w));

    return 1;
}

/* ↓ :lengthSqr() ↓ */
static int quatLengthSqr(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");

    lua_pushnumber(L, x*x + y*y + z*z + w*w);

    return 1;
}

/* ↓ :normalize() — mutates in-place, returns self ↓ */
static int quatNormalize(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");
    float len = sqrtf(x*x + y*y + z*z + w*w);

    if (len > 0.0f) {
        setFieldNumber(L, "x", x / len);
        setFieldNumber(L, "y", y / len);
        setFieldNumber(L, "z", z / len);
        setFieldNumber(L, "w", w / len);
    } else {
        setFieldNumber(L, "x", 0.0f);
        setFieldNumber(L, "y", 0.0f);
        setFieldNumber(L, "z", 0.0f);
        setFieldNumber(L, "w", 1.0f);
    }

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getNormalized() → new Quaternion ↓ */
static int quatGetNormalized(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");
    float len = sqrtf(x*x + y*y + z*z + w*w);

    if (len > 0.0f) pushQuat(L, x/len, y/len, z/len, w/len);
    else            pushQuat(L, 0.0f, 0.0f, 0.0f, 1.0f);

    return 1;
}

/* ↓ :conjugate() — mutates in-place, returns self ↓ */
static int quatConjugate(lua_State *L) {
    setFieldNumber(L, "x", -getfieldf(L, 1, "x"));
    setFieldNumber(L, "y", -getfieldf(L, 1, "y"));
    setFieldNumber(L, "z", -getfieldf(L, 1, "z"));
    setFieldNumber(L, "w",  getfieldf(L, 1, "w"));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getConjugate() → new Quaternion ↓ */
static int quatGetConjugate(lua_State *L) {
    pushQuat(L,
        -getfieldf(L, 1, "x"),
        -getfieldf(L, 1, "y"),
        -getfieldf(L, 1, "z"),
         getfieldf(L, 1, "w")
    );

    return 1;
}

/* ↓ :invert() — full inverse (conjugate / lenSqr), mutates in-place, returns self ↓ */
static int quatInvert(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");
    float lenSqr = x*x + y*y + z*z + w*w;

    if (lenSqr > 0.0f) {
        setFieldNumber(L, "x", -x / lenSqr);
        setFieldNumber(L, "y", -y / lenSqr);
        setFieldNumber(L, "z", -z / lenSqr);
        setFieldNumber(L, "w",  w / lenSqr);
    }

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getInverse() → new Quaternion ↓ */
static int quatGetInverse(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");
    float lenSqr = x*x + y*y + z*z + w*w;

    if (lenSqr > 0.0f) 
        pushQuat(L, -x/lenSqr, -y/lenSqr, -z/lenSqr, w/lenSqr);
    else               
        pushQuat(L, 0.0f, 0.0f, 0.0f, 1.0f);

    return 1;
}

/* ↓ :slerp(other, t) — spherical linear interpolation ↓ */
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
    float theta = theta0 * t;
    float sinT = sinf(theta);
    float sinT0 = sinf(theta0);
    float s0 = cosf(theta) - dot * sinT / sinT0;
    float s1 = sinT / sinT0;

    pushQuat(L,
        s0*ax + s1*bx,
        s0*ay + s1*by,
        s0*az + s1*bz,
        s0*aw + s1*bw
    );

    return 1;
}

/* ↓ :lerp(other, t) — linear interpolation, result is normalized ↓ */
static int quatLerp(lua_State *L) {
    float ax = getfieldf(L, 1, "x"), ay = getfieldf(L, 1, "y");
    float az = getfieldf(L, 1, "z"), aw = getfieldf(L, 1, "w");
    float bx = getfieldf(L, 2, "x"), by = getfieldf(L, 2, "y");
    float bz = getfieldf(L, 2, "z"), bw = getfieldf(L, 2, "w");
    float t = (float)luaL_checknumber(L, 3);

    float rx = ax + t*(bx-ax), ry = ay + t*(by-ay);
    float rz = az + t*(bz-az), rw = aw + t*(bw-aw);
    float len = sqrtf(rx*rx + ry*ry + rz*rz + rw*rw);

    if (len > 0.0f) 
        ushQuat(L, rx/len, ry/len, rz/len, rw/len);
    else            
        ushQuat(L, 0.0f, 0.0f, 0.0f, 1.0f);

    return 1;
}

/* ↓ :rotateVector(vec) → new Vector rotated by this quaternion ↓ */
static int quatRotateVector(lua_State *L) {
    float qx = getfieldf(L, 1, "x"), qy = getfieldf(L, 1, "y");
    float qz = getfieldf(L, 1, "z"), qw = getfieldf(L, 1, "w");
    float vx = getfieldf(L, 2, "x"), vy = getfieldf(L, 2, "y"), vz = getfieldf(L, 2, "z");

    /* ↓ t = 2 * cross(q.xyz, v), result = v + q.w * t + cross(q.xyz, t) ↓ */
    float tx = 2.0f*(qy*vz - qz*vy);
    float ty = 2.0f*(qz*vx - qx*vz);
    float tz = 2.0f*(qx*vy - qy*vx);

    lua_newtable(L);
    setFieldNumber(L, "x", vx + qw*tx + qy*tz - qz*ty);
    setFieldNumber(L, "y", vy + qw*ty + qz*tx - qx*tz);
    setFieldNumber(L, "z", vz + qw*tz + qx*ty - qy*tx);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1))
        lua_setmetatable(L, -2);
    else
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getForward() → Vector ↓ */
static int quatGetForward(lua_State *L) {
    float qx = getfieldf(L, 1, "x"), qy = getfieldf(L, 1, "y");
    float qz = getfieldf(L, 1, "z"), qw = getfieldf(L, 1, "w");

    /* ↓ rotate (1, 0, 0) by this quaternion ↓ */
    float tx = 0.0f, ty = 2.0f*qz, tz = -2.0f*qy;

    lua_newtable(L);
    setFieldNumber(L, "x", 1.0f + qw*tx + qy*tz - qz*ty);
    setFieldNumber(L, "y",        qw*ty + qz*tx - qx*tz);
    setFieldNumber(L, "z",        qw*tz + qx*ty - qy*tx);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1))
        lua_setmetatable(L, -2);
    else
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getRight() → Vector ↓ */
static int quatGetRight(lua_State *L) {
    float qx = getfieldf(L, 1, "x"), qy = getfieldf(L, 1, "y");
    float qz = getfieldf(L, 1, "z"), qw = getfieldf(L, 1, "w");

    /* ↓ rotate (0, 1, 0) by this quaternion ↓ */
    float tx = -2.0f*qz, ty = 0.0f, tz = 2.0f*qx;

    lua_newtable(L);
    setFieldNumber(L, "x",        qw*tx + qy*tz - qz*ty);
    setFieldNumber(L, "y", 1.0f + qw*ty + qz*tx - qx*tz);
    setFieldNumber(L, "z",        qw*tz + qx*ty - qy*tx);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1))
        lua_setmetatable(L, -2);
    else
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getUp() → Vector ↓ */
static int quatGetUp(lua_State *L) {
    float qx = getfieldf(L, 1, "x"), qy = getfieldf(L, 1, "y");
    float qz = getfieldf(L, 1, "z"), qw = getfieldf(L, 1, "w");

    /* ↓ rotate (0, 0, 1) by this quaternion ↓ */
    float tx = 2.0f*qy, ty = -2.0f*qx, tz = 0.0f;

    lua_newtable(L);
    setFieldNumber(L, "x",        qw*tx + qy*tz - qz*ty);
    setFieldNumber(L, "y",        qw*ty + qz*tx - qx*tz);
    setFieldNumber(L, "z", 1.0f + qw*tz + qx*ty - qy*tx);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1))
        lua_setmetatable(L, -2);
    else
        lua_pop(L, 1);

    return 1;
}

/* ↓ :isZero() ↓ */
static int quatIsZero(lua_State *L) {
    lua_pushboolean(L,
        getfieldf(L, 1, "x") == 0.0f &&
        getfieldf(L, 1, "y") == 0.0f &&
        getfieldf(L, 1, "z") == 0.0f &&
        getfieldf(L, 1, "w") == 0.0f
    );

    return 1;
}

/* ↓ :round([decimals]) → new Quaternion ↓ */
static int quatRound(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z"), w = getfieldf(L, 1, "w");
    float mul = 1.0f;

    if (!lua_isnoneornil(L, 2)) {
        int dec = (int)lua_tointeger(L, 2);

        for (int i = 0; i < dec; i++)
            mul *= 10.0f;
    }

    pushQuat(L,
        roundf(x * mul) / mul,
        roundf(y * mul) / mul,
        roundf(z * mul) / mul,
        roundf(w * mul) / mul
    );

    return 1;
}

static const luaL_Reg quatMethods[] = {
    {"set",              quatSet},
    {"setIdentity",      quatSetIdentity},
    {"isIdentity",       quatIsIdentity},
    {"clone",            quatClone},
    {"toAngle",          quatToAngle},
    {"getAngle",         quatGetAngle},
    {"setAngle",         quatSetAngle},
    {"getMatrix",        quatGetMatrix},
    {"setMatrix",        quatSetMatrix},
    {"setAxisAngle",     quatSetAxisAngle},
    {"getAxisAngle",     quatGetAxisAngle},
    {"rotateAroundAxis", quatRotateAroundAxis},
    {"dot",              quatDot},
    {"length",           quatLength},
    {"lengthSqr",        quatLengthSqr},
    {"normalize",        quatNormalize},
    {"getNormalized",    quatGetNormalized},
    {"conjugate",        quatConjugate},
    {"getConjugate",     quatGetConjugate},
    {"invert",           quatInvert},
    {"getInverse",       quatGetInverse},
    {"slerp",            quatSlerp},
    {"lerp",             quatLerp},
    {"rotateVector",     quatRotateVector},
    {"getForward",       quatGetForward},
    {"getRight",         quatGetRight},
    {"getUp",            quatGetUp},
    {"isZero",           quatIsZero},
    {"round",            quatRound},

    {NULL, NULL}
};

static const luaL_Reg quatMeta[] = {
    {"__tostring", toStringQuat},
    {"__add",      quatAdd},
    {"__sub",      quatSub},
    {"__mul",      quatMul},
    {"__div",      quatDiv},
    {"__unm",      quatUnm},
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