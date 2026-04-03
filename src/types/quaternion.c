#include "common.h"

#include <math.h>

static const luaL_Reg quatMethods[];
static const luaL_Reg quatMeta[];

struct VanirQuat *pushQuat(lua_State *L, float x, float y, float z, float w) {
    struct VanirQuat *q = (struct VanirQuat *)lua_newuserdata(L, sizeof(struct VanirQuat));
    
    q->x = x;
    q->y = y;
    q->z = z;
    q->w = w;

    addMethodsUD(L, "vanir.Quaternion", quatMethods, quatMeta);

    return q;
}

/* ↓ __tostring ↓ */
static int toStringQuat(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    lua_pushfstring(L, "(%f, %f, %f, %f)", q->x, q->y, q->z, q->w);

    return 1;
}

/* ↓ __add ↓ */
static int quatAdd(lua_State *L) {
    struct VanirQuat *a = checkQuat(L, 1);
    struct VanirQuat *b = checkQuat(L, 2);

    pushQuat(L, a->x + b->x, a->y + b->y, a->z + b->z, a->w + b->w);

    return 1;
}

/* ↓ __sub ↓ */
static int quatSub(lua_State *L) {
    struct VanirQuat *a = checkQuat(L, 1);
    struct VanirQuat *b = checkQuat(L, 2);

    pushQuat(L, a->x - b->x, a->y - b->y, a->z - b->z, a->w - b->w);

    return 1;
}

/* ↓ __mul: quat * quat — Hamilton product  OR  quat * scalar ↓ */
static int quatMul(lua_State *L) {
    if (lua_isnumber(L, 2)) {
        struct VanirQuat *q = checkQuat(L, 1);
        float s = (float)lua_tonumber(L, 2);

        pushQuat(L, q->x * s, q->y * s, q->z * s, q->w * s);

        return 1;
    }

    struct VanirQuat *a = checkQuat(L, 1);
    struct VanirQuat *b = checkQuat(L, 2);

    pushQuat(L,
        a->w*b->x + a->x*b->w + a->y*b->z - a->z*b->y,
        a->w*b->y - a->x*b->z + a->y*b->w + a->z*b->x,
        a->w*b->z + a->x*b->y - a->y*b->x + a->z*b->w,
        a->w*b->w - a->x*b->x - a->y*b->y - a->z*b->z
    );

    return 1;
}

/* ↓ __div: quat / scalar ↓ */
static int quatDiv(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    float s = (float)luaL_checknumber(L, 2);

    pushQuat(L, q->x / s, q->y / s, q->z / s, q->w / s);

    return 1;
}

/* ↓ __unm ↓ */
static int quatUnm(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    pushQuat(L, -q->x, -q->y, -q->z, -q->w);

    return 1;
}

/* ↓ __eq ↓ */
static int quatEq(lua_State *L) {
    struct VanirQuat *a = checkQuat(L, 1);
    struct VanirQuat *b = checkQuat(L, 2);

    lua_pushboolean(L, a->x == b->x && a->y == b->y && a->z == b->z && a->w == b->w);

    return 1;
}

/* ↓ :set(x, y, z, w) — mutates in-place, returns self ↓ */
static int quatSet(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    q->x = (float)luaL_checknumber(L, 2);
    q->y = (float)luaL_checknumber(L, 3);
    q->z = (float)luaL_checknumber(L, 4);
    q->w = (float)luaL_checknumber(L, 5);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setIdentity() — mutates in-place, returns self ↓ */
static int quatSetIdentity(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    q->x = 0.0f;
    q->y = 0.0f;
    q->z = 0.0f;
    q->w = 1.0f;

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :isIdentity() ↓ */
static int quatIsIdentity(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    lua_pushboolean(L, q->x == 0.0f && q->y == 0.0f && q->z == 0.0f && q->w == 1.0f);

    return 1;
}

/* ↓ :clone() ↓ */
static int quatClone(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    pushQuat(L, q->x, q->y, q->z, q->w);

    return 1;
}

/* ↓ quat:toAngle() — converts to Angle (roll/pitch/yaw) in degrees ↓ */
static int quatToAngle(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    /* ↓ ZXY order: yaw, roll, pitch ↓ */
    float sinR = 2.0f*(q->w*q->x + q->y*q->z),  cosR = 1.0f - 2.0f*(q->x*q->x + q->y*q->y);
    float sinP = 2.0f*(q->w*q->y - q->z*q->x);
    float sinY = 2.0f*(q->w*q->z + q->x*q->y),  cosY = 1.0f - 2.0f*(q->y*q->y + q->z*q->z);

    float roll  = atan2f(sinR, cosR) * (180.0f / 3.14159265f);
    float pitch = (fabsf(sinP) >= 1.0f) ? copysignf(90.0f, sinP) : asinf(sinP) * (180.0f / 3.14159265f);
    float yaw   = atan2f(sinY, cosY) * (180.0f / 3.14159265f);

    pushAngle(L, pitch, yaw, roll);

    return 1;
}

/* ↓ :getAngle() → Angle (roll/pitch/yaw in degrees) ↓ */
static int quatGetAngle(lua_State *L) {
    return quatToAngle(L);
}

/* ↓ :setAngle(angle) — sets quat from Angle (pitch/yaw/roll in degrees), mutates in-place, returns self ↓ */
static int quatSetAngle(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    struct VanirAng *a = checkAng(L, 2);

    float pitch = degToRad(a->p) * 0.5f;
    float yaw = degToRad(a->y) * 0.5f;
    float roll = degToRad(a->r) * 0.5f;

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw),   sy = sinf(yaw);
    float cr = cosf(roll),  sr = sinf(roll);

    q->x = sp*cy*cr + cp*sy*sr;
    q->y = cp*sy*cr - sp*cy*sr;
    q->z = cp*cy*sr - sp*sy*cr;
    q->w = cp*cy*cr + sp*sy*sr;

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getMatrix() → Matrix (3x3 rotation matrix) ↓ */
static int quatGetMatrix(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    float x2 = q->x+q->x, y2 = q->y+q->y, z2 = q->z+q->z;
    float xx = q->x*x2, xy = q->x*y2, xz = q->x*z2;
    float yy = q->y*y2, yz = q->y*z2, zz = q->z*z2;
    float wx = q->w*x2, wy = q->w*y2, wz = q->w*z2;

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
    struct VanirQuat *q = checkQuat(L, 1);
    float m[9];

    for (int i = 0; i < 9; i++) {
        lua_rawgeti(L, 2, i + 1);

        m[i] = (float)lua_tonumber(L, -1);

        lua_pop(L, 1);
    }

    /* ↓ Shepperd's method ↓ */
    float trace = m[0] + m[4] + m[8];

    if (trace > 0.0f) {
        float s = 0.5f / sqrtf(trace + 1.0f);
        q->w = 0.25f / s;
        q->x = (m[7] - m[5]) * s;
        q->y = (m[2] - m[6]) * s;
        q->z = (m[3] - m[1]) * s;
    } else if (m[0] > m[4] && m[0] > m[8]) {
        float s = 2.0f * sqrtf(1.0f + m[0] - m[4] - m[8]);
        q->w = (m[7] - m[5]) / s;
        q->x = 0.25f * s;
        q->y = (m[1] + m[3]) / s;
        q->z = (m[2] + m[6]) / s;
    } else if (m[4] > m[8]) {
        float s = 2.0f * sqrtf(1.0f + m[4] - m[0] - m[8]);
        q->w = (m[2] - m[6]) / s;
        q->x = (m[1] + m[3]) / s;
        q->y = 0.25f * s;
        q->z = (m[5] + m[7]) / s;
    } else {
        float s = 2.0f * sqrtf(1.0f + m[8] - m[0] - m[4]);
        q->w = (m[3] - m[1]) / s;
        q->x = (m[2] + m[6]) / s;
        q->y = (m[5] + m[7]) / s;
        q->z = 0.25f * s;
    }

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setAxisAngle(axis_vec, degrees) — mutates in-place, returns self ↓ */
static int quatSetAxisAngle(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    struct VanirVec *axis = checkVec(L, 2);
    float half = degToRad((float)luaL_checknumber(L, 3)) * 0.5f;
    float s = sinf(half);

    q->x = axis->x * s;
    q->y = axis->y * s;
    q->z = axis->z * s;
    q->w = cosf(half);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getAxisAngle() → axis_vec, degrees ↓ */
static int quatGetAxisAngle(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    float sinHalf = sqrtf(q->x*q->x + q->y*q->y + q->z*q->z);
    float degrees = 2.0f * atan2f(sinHalf, q->w) * (180.0f / 3.14159265f);
    float ax, ay, az;

    if (sinHalf > 0.0001f) {
        ax = q->x / sinHalf;
        ay = q->y / sinHalf;
        az = q->z / sinHalf;
    } else {
        /* ↓ identity — axis is arbitrary ↓ */
        ax = 0.0f;
        ay = 0.0f;
        az = 1.0f;
    }

    struct VanirVec *v = (struct VanirVec *)lua_newuserdata(L, sizeof(struct VanirVec));

    v->x = ax;
    v->y = ay;
    v->z = az;

    luaL_setmetatable(L, "vanir.Vector");

    lua_pushnumber(L, degrees);

    return 2;
}

/* ↓ :rotateAroundAxis(axis_vec, degrees) → new Quaternion ↓ */
static int quatRotateAroundAxis(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    struct VanirVec *axis = checkVec(L, 2);
    float half = degToRad((float)luaL_checknumber(L, 3)) * 0.5f;
    float s = sinf(half);

    float bx = axis->x * s, by = axis->y * s, bz = axis->z * s, bw = cosf(half);

    pushQuat(L,
        q->w*bx + q->x*bw + q->y*bz - q->z*by,
        q->w*by - q->x*bz + q->y*bw + q->z*bx,
        q->w*bz + q->x*by - q->y*bx + q->z*bw,
        q->w*bw - q->x*bx - q->y*by - q->z*bz
    );

    return 1;
}

/* ↓ :dot(other) ↓ */
static int quatDot(lua_State *L) {
    struct VanirQuat *a = checkQuat(L, 1);
    struct VanirQuat *b = checkQuat(L, 2);

    lua_pushnumber(L, a->x*b->x + a->y*b->y + a->z*b->z + a->w*b->w);

    return 1;
}

/* ↓ :length() — magnitude ↓ */
static int quatLength(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    lua_pushnumber(L, sqrtf(q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w));

    return 1;
}

/* ↓ :lengthSqr() ↓ */
static int quatLengthSqr(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    lua_pushnumber(L, q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w);

    return 1;
}

/* ↓ :normalize() — mutates in-place, returns self ↓ */
static int quatNormalize(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    float len = sqrtf(q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w);

    if (len > 0.0f) {
        q->x /= len; q->y /= len; q->z /= len; q->w /= len;
    } else {
        q->x = 0.0f; q->y = 0.0f; q->z = 0.0f; q->w = 1.0f;
    }

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getNormalized() → new Quaternion ↓ */
static int quatGetNormalized(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    float len = sqrtf(q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w);

    if (len > 0.0f) pushQuat(L, q->x/len, q->y/len, q->z/len, q->w/len);
    else            pushQuat(L, 0.0f, 0.0f, 0.0f, 1.0f);

    return 1;
}

/* ↓ :conjugate() — mutates in-place, returns self ↓ */
static int quatConjugate(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    q->x = -q->x;
    q->y = -q->y;
    q->z = -q->z;

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getConjugate() → new Quaternion ↓ */
static int quatGetConjugate(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    pushQuat(L, -q->x, -q->y, -q->z, q->w);

    return 1;
}

/* ↓ :invert() — full inverse (conjugate / lenSqr), mutates in-place, returns self ↓ */
static int quatInvert(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    float lenSqr = q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w;

    if (lenSqr > 0.0f) {
        q->x = -q->x / lenSqr;
        q->y = -q->y / lenSqr;
        q->z = -q->z / lenSqr;
        q->w =  q->w / lenSqr;
    }

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getInverse() → new Quaternion ↓ */
static int quatGetInverse(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    float lenSqr = q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w;

    if (lenSqr > 0.0f)
        pushQuat(L, -q->x/lenSqr, -q->y/lenSqr, -q->z/lenSqr, q->w/lenSqr);
    else
        pushQuat(L, 0.0f, 0.0f, 0.0f, 1.0f);

    return 1;
}

/* ↓ :slerp(other, t) — spherical linear interpolation ↓ */
static int quatSlerp(lua_State *L) {
    struct VanirQuat *a = checkQuat(L, 1);
    struct VanirQuat *b = checkQuat(L, 2);
    float t = (float)luaL_checknumber(L, 3);

    float bx = b->x, by = b->y, bz = b->z, bw = b->w;
    float dot = a->x*bx + a->y*by + a->z*bz + a->w*bw;

    /* ↓ flip second quat if dot is negative to take the short arc ↓ */
    if (dot < 0.0f) {
        bx=-bx; by=-by; bz=-bz; bw=-bw;
        dot=-dot;
    }

    if (dot > 0.9995f) {
        float rx = a->x + t*(bx-a->x), ry = a->y + t*(by-a->y);
        float rz = a->z + t*(bz-a->z), rw = a->w + t*(bw-a->w);
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
        s0*a->x + s1*bx,
        s0*a->y + s1*by,
        s0*a->z + s1*bz,
        s0*a->w + s1*bw
    );

    return 1;
}

/* ↓ :lerp(other, t) — linear interpolation, result is normalized ↓ */
static int quatLerp(lua_State *L) {
    struct VanirQuat *a = checkQuat(L, 1);
    struct VanirQuat *b = checkQuat(L, 2);
    float t = (float)luaL_checknumber(L, 3);

    float rx = a->x + t*(b->x-a->x), ry = a->y + t*(b->y-a->y);
    float rz = a->z + t*(b->z-a->z), rw = a->w + t*(b->w-a->w);
    float len = sqrtf(rx*rx + ry*ry + rz*rz + rw*rw);

    if (len > 0.0f)
        pushQuat(L, rx/len, ry/len, rz/len, rw/len);
    else
        pushQuat(L, 0.0f, 0.0f, 0.0f, 1.0f);

    return 1;
}

/* ↓ :rotateVector(vec) → new Vector rotated by this quaternion ↓ */
static int quatRotateVector(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    struct VanirVec  *iv = checkVec(L, 2);

    /* ↓ t = 2 * cross(q.xyz, v), result = v + q.w * t + cross(q.xyz, t) ↓ */
    float tx = 2.0f*(q->y*iv->z - q->z*iv->y);
    float ty = 2.0f*(q->z*iv->x - q->x*iv->z);
    float tz = 2.0f*(q->x*iv->y - q->y*iv->x);

    struct VanirVec *ov = (struct VanirVec *)lua_newuserdata(L, sizeof(struct VanirVec));

    ov->x = iv->x + q->w*tx + q->y*tz - q->z*ty;
    ov->y = iv->y + q->w*ty + q->z*tx - q->x*tz;
    ov->z = iv->z + q->w*tz + q->x*ty - q->y*tx;

    luaL_setmetatable(L, "vanir.Vector");

    return 1;
}

/* ↓ :getForward() → Vector ↓ */
static int quatGetForward(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    /* ↓ rotate (1, 0, 0) by this quaternion ↓ */
    float tx = 0.0f, ty = 2.0f*q->z, tz = -2.0f*q->y;

    struct VanirVec *v = (struct VanirVec *)lua_newuserdata(L, sizeof(struct VanirVec));

    v->x = 1.0f + q->w*tx + q->y*tz - q->z*ty;
    v->y =        q->w*ty + q->z*tx - q->x*tz;
    v->z =        q->w*tz + q->x*ty - q->y*tx;

    luaL_setmetatable(L, "vanir.Vector");

    return 1;
}

/* ↓ :getRight() → Vector ↓ */
static int quatGetRight(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    /* ↓ rotate (0, 1, 0) by this quaternion ↓ */
    float tx = -2.0f*q->z, ty = 0.0f, tz = 2.0f*q->x;

    struct VanirVec *v = (struct VanirVec *)lua_newuserdata(L, sizeof(struct VanirVec));

    v->x =        q->w*tx + q->y*tz - q->z*ty;
    v->y = 1.0f + q->w*ty + q->z*tx - q->x*tz;
    v->z =        q->w*tz + q->x*ty - q->y*tx;

    luaL_setmetatable(L, "vanir.Vector");

    return 1;
}

/* ↓ :getUp() → Vector ↓ */
static int quatGetUp(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    /* ↓ rotate (0, 0, 1) by this quaternion ↓ */
    float tx = 2.0f*q->y, ty = -2.0f*q->x, tz = 0.0f;

    struct VanirVec *v = (struct VanirVec *)lua_newuserdata(L, sizeof(struct VanirVec));

    v->x =        q->w*tx + q->y*tz - q->z*ty;
    v->y =        q->w*ty + q->z*tx - q->x*tz;
    v->z = 1.0f + q->w*tz + q->x*ty - q->y*tx;

    luaL_setmetatable(L, "vanir.Vector");

    return 1;
}

/* ↓ :isZero() ↓ */
static int quatIsZero(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);

    lua_pushboolean(L, q->x == 0.0f && q->y == 0.0f && q->z == 0.0f && q->w == 0.0f);

    return 1;
}

/* ↓ :round([decimals]) → new Quaternion ↓ */
static int quatRound(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    float mul = 1.0f;

    if (!lua_isnoneornil(L, 2)) {
        int dec = (int)lua_tointeger(L, 2);

        for (int i = 0; i < dec; i++)
            mul *= 10.0f;
    }

    pushQuat(L,
        roundf(q->x * mul) / mul,
        roundf(q->y * mul) / mul,
        roundf(q->z * mul) / mul,
        roundf(q->w * mul) / mul
    );

    return 1;
}

/* ↓ __index — expose x/y/z/w fields from userdata to Lua ↓ */
static int quatIndex(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    const char *key = luaL_checkstring(L, 2);

    if (key[1] == '\0') {
        if (key[0] == 'x') { lua_pushnumber(L, q->x); return 1; }
        if (key[0] == 'y') { lua_pushnumber(L, q->y); return 1; }
        if (key[0] == 'z') { lua_pushnumber(L, q->z); return 1; }
        if (key[0] == 'w') { lua_pushnumber(L, q->w); return 1; }
    }

    /* ↓ fall through to method table ↓ */
    vanirUD_indexFallback(L, "vanir.Quaternion", key);

    return 1;
}

/* ↓ __newindex — allow q.x = n style assignment ↓ */
static int quatNewIndex(lua_State *L) {
    struct VanirQuat *q = checkQuat(L, 1);
    const char *key = luaL_checkstring(L, 2);
    float val = (float)luaL_checknumber(L, 3);

    if (key[1] == '\0') {
        if (key[0] == 'x') { q->x = val; return 0; }
        if (key[0] == 'y') { q->y = val; return 0; }
        if (key[0] == 'z') { q->z = val; return 0; }
        if (key[0] == 'w') { q->w = val; return 0; }
    }

    return luaL_error(L, "Quaternion has no field '%s'", key);
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
    {"__index",    quatIndex},
    {"__newindex", quatNewIndex},

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