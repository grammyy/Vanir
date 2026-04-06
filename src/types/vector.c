#include "common.h"

#include <math.h>

static const luaL_Reg vecMethods[];
static const luaL_Reg vecMeta[];

struct VanirVec *pushVec(lua_State *L, float x, float y, float z) {
    struct VanirVec *v = (struct VanirVec *)lua_newuserdata(L, sizeof(struct VanirVec));
    v->x = x;
    v->y = y;
    v->z = z;

    addMethodsUD(L, "vanir.Vector", vecMethods, vecMeta);
    
    return v;
}

/* ↓ __tostring ↓ */
static int toStringVec(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    lua_pushfstring(L, "(%f, %f, %f)", v->x, v->y, v->z);

    return 1;
}

/* ↓ __add ↓ */
static int vecAdd(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);

    pushVec(L, a->x + b->x, a->y + b->y, a->z + b->z);

    return 1;
}

/* ↓ __sub ↓ */
static int vecSub(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);

    pushVec(L, a->x - b->x, a->y - b->y, a->z - b->z);

    return 1;
}

/* ↓ __mul: vec * scalar  or  scalar * vec ↓ */
static int vecMul(lua_State *L) {
    float x, y, z, s;

    if (lua_isnumber(L, 1)) {
        s = (float)lua_tonumber(L, 1);
        struct VanirVec *v = checkVec(L, 2);
        x = v->x; y = v->y; z = v->z;
    } else {
        struct VanirVec *v = checkVec(L, 1);
        x = v->x; y = v->y; z = v->z;
        s = (float)luaL_checknumber(L, 2);
    }

    pushVec(L, x * s, y * s, z * s);

    return 1;
}

/* ↓ __div: vec / scalar ↓ */
static int vecDiv(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    float s = (float)luaL_checknumber(L, 2);

    pushVec(L, v->x / s, v->y / s, v->z / s);

    return 1;
}

/* ↓ __unm ↓ */
static int vecUnm(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    pushVec(L, -v->x, -v->y, -v->z);

    return 1;
}

/* ↓ __eq ↓ */
static int vecEq(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);

    lua_pushboolean(L, a->x == b->x && a->y == b->y && a->z == b->z);

    return 1;
}

/* ↓ __len: #vec → magnitude ↓ */
static int vecLen(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    lua_pushnumber(L, sqrtf(v->x*v->x + v->y*v->y + v->z*v->z));

    return 1;
}

/* ↓ :dot(other) ↓ */
static int vecDot(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);

    lua_pushnumber(L, a->x*b->x + a->y*b->y + a->z*b->z);

    return 1;
}

/* ↓ :cross(other) → Vector ↓ */
static int vecCross(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);

    pushVec(L,
        a->y*b->z - a->z*b->y,
        a->z*b->x - a->x*b->z,
        a->x*b->y - a->y*b->x
    );

    return 1;
}

/* ↓ :getLength() ↓ */
static int vecGetLength(lua_State *L) {
    return vecLen(L);
}

/* ↓ :getLengthSqr() ↓ */
static int vecGetLengthSqr(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    lua_pushnumber(L, v->x*v->x + v->y*v->y + v->z*v->z);

    return 1;
}

/* ↓ :getLength2D() ↓ */
static int vecGetLength2D(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    lua_pushnumber(L, sqrtf(v->x*v->x + v->y*v->y));

    return 1;
}

/* ↓ :getLength2DSqr() ↓ */
static int vecGetLength2DSqr(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    lua_pushnumber(L, v->x*v->x + v->y*v->y);

    return 1;
}

/* ↓ :getDistance(other) ↓ */
static int vecGetDistance(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);
    float dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;

    lua_pushnumber(L, sqrtf(dx*dx + dy*dy + dz*dz));

    return 1;
}

/* ↓ :getDistanceSqr(other) ↓ */
static int vecGetDistanceSqr(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);
    float dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;

    lua_pushnumber(L, dx*dx + dy*dy + dz*dz);

    return 1;
}

/* ↓ :getNormalized() → new Vector ↓ */
static int vecGetNormalized(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    float len = sqrtf(v->x*v->x + v->y*v->y + v->z*v->z);

    if (len > 0.0f)
        pushVec(L, v->x / len, v->y / len, v->z / len);
    else
        pushVec(L, 0.0f, 0.0f, 0.0f);

    return 1;
}

/* ↓ :normalize() — mutates in-place, returns self ↓ */
static int vecNormalize(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    float len = sqrtf(v->x*v->x + v->y*v->y + v->z*v->z);

    if (len > 0.0f) {
        v->x /= len;
        v->y /= len;
        v->z /= len;
    } else {
        v->x = 0.0f;
        v->y = 0.0f;
        v->z = 0.0f;
    }

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :isZero([tolerance]) ↓ */
static int vecIsZero(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    if (!lua_isnoneornil(L, 2)) {
        float tol = (float)lua_tonumber(L, 2);

        lua_pushboolean(L, fabsf(v->x) <= tol && fabsf(v->y) <= tol && fabsf(v->z) <= tol);
    } else {
        lua_pushboolean(L, v->x == 0.0f && v->y == 0.0f && v->z == 0.0f);
    }

    return 1;
}

/* ↓ :isEqualTol(other, tol) ↓ */
static int vecIsEqualTol(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);
    float tol = (float)luaL_checknumber(L, 3);

    lua_pushboolean(L,
        fabsf(a->x - b->x) <= tol &&
        fabsf(a->y - b->y) <= tol &&
        fabsf(a->z - b->z) <= tol
    );

    return 1;
}

/* ↓ :getAngle() → Angle (pitch/yaw from direction) ↓ */
static int vecGetAngle(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    float pitch = radToDeg(-atan2f(v->z, sqrtf(v->x*v->x + v->y*v->y)));
    float yaw = radToDeg(atan2f(v->y, v->x));

    struct VanirAng *a = (struct VanirAng *)lua_newuserdata(L, sizeof(struct VanirAng));
    a->p = pitch;
    a->y = yaw;
    a->r = 0.0f;
    luaL_setmetatable(L, "vanir.Angle");

    return 1;
}

/* ↓ :getAngleEx(up_vec) → Angle with roll derived from up reference ↓ */
static int vecGetAngleEx(lua_State *L) {
    struct VanirVec *fwd = checkVec(L, 1);
    struct VanirVec *up  = checkVec(L, 2);

    float fx = fwd->x, fy = fwd->y, fz = fwd->z;
    float flen = sqrtf(fx*fx + fy*fy + fz*fz);

    if (flen > 0.0f) { fx /= flen; fy /= flen; fz /= flen; }

    float pitch = radToDeg(-atan2f(fz, sqrtf(fx*fx + fy*fy)));
    float yaw = radToDeg(atan2f(fy, fx));

    /* ↓ compute right = forward x up, then actual up = right x forward ↓ */
    float rx = fy*up->z - fz*up->y;
    float ry = fz*up->x - fx*up->z;
    float rz = fx*up->y - fy*up->x;

    float rlen = sqrtf(rx*rx + ry*ry + rz*rz);

    if (rlen > 0.0f) { rx /= rlen; ry /= rlen; rz /= rlen; }

    float aupx = ry*fz - rz*fy;
    float aupy = rz*fx - rx*fz;
    float aupz = rx*fy - ry*fx;

    float roll = radToDeg(atan2f(-rx*aupz + rz*aupx, aupy));

    struct VanirAng *a = (struct VanirAng *)lua_newuserdata(L, sizeof(struct VanirAng));
    a->p = pitch;
    a->y = yaw;
    a->r = roll;
    luaL_setmetatable(L, "vanir.Angle");

    return 1;
}

/* ↓ :add(other) — mutates in-place, returns self ↓ */
static int vecAddInPlace(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);

    a->x += b->x;
    a->y += b->y;
    a->z += b->z;

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :sub(other) — mutates in-place, returns self ↓ */
static int vecSubInPlace(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);

    a->x -= b->x;
    a->y -= b->y;
    a->z -= b->z;

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :mul(scalar) — mutates in-place, returns self ↓ */
static int vecMulInPlace(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    float s = (float)luaL_checknumber(L, 2);

    v->x *= s;
    v->y *= s;
    v->z *= s;

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :div(scalar) — mutates in-place, returns self ↓ */
static int vecDivInPlace(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    float s = (float)luaL_checknumber(L, 2);

    v->x /= s;
    v->y /= s;
    v->z /= s;

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :vmul(other_vec) — component-wise multiply, returns new Vector ↓ */
static int vecVmul(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);

    pushVec(L, a->x * b->x, a->y * b->y, a->z * b->z);

    return 1;
}

/* ↓ :vdiv(other_vec) — component-wise divide, returns new Vector ↓ */
static int vecVdiv(lua_State *L) {
    struct VanirVec *a = checkVec(L, 1);
    struct VanirVec *b = checkVec(L, 2);

    pushVec(L, a->x / b->x, a->y / b->y, a->z / b->z);

    return 1;
}

/* ↓ :setZero() — mutates in-place, returns self ↓ */
static int vecSetZero(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    v->x = 0.0f;
    v->y = 0.0f;
    v->z = 0.0f;

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setX/Y/Z(v) ↓ */
static int vecSetX(lua_State *L) { setField(checkVec, x); }
static int vecSetY(lua_State *L) { setField(checkVec, y); }
static int vecSetZ(lua_State *L) { setField(checkVec, z); }

/* ↓ :set(x, y, z) — mutates in-place, returns self ↓ */
static int vecSet(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    v->x = (float)luaL_checknumber(L, 2);
    v->y = (float)luaL_checknumber(L, 3);
    v->z = (float)luaL_checknumber(L, 4);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :clone() ↓ */
static int vecClone(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);

    pushVec(L, v->x, v->y, v->z);

    return 1;
}

/* ↓ :round([decimals]) → new Vector ↓ */
static int vecRound(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    float mul = roundMultiplier(L, 2);

    pushVec(L,
        roundf(v->x * mul) / mul,
        roundf(v->y * mul) / mul,
        roundf(v->z * mul) / mul
    );

    return 1;
}

/* ↓ rotate(angle) — rotates vector in-place by Angle using quaternion math ↓ */
static int vecRotate(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    struct VanirAng *a;

    if (luaL_testudata(L, 2, "vanir.Angle")) {
        a = (struct VanirAng *)lua_touserdata(L, 2);
    } else {
        luaL_checktype(L, 2, LUA_TTABLE);

        static struct VanirAng temp;
        temp.p = (float)getField(L, 2, "p");
        temp.y = (float)getField(L, 2, "y");
        temp.r = (float)getField(L, 2, "r");
        a = &temp;
    }

    /* ↓ build quaternion from Angle (ZYX: yaw(Z), pitch(Y), roll(X)) ↓ */
    float qw, qx, qy, qz;
    angToQuat(a, &qw, &qx, &qy, &qz);

    /* ↓ t = 2 * cross(q.xyz, v), result = v + q.w * t + cross(q.xyz, t) ↓ */
    float tx = 2.0f*(qy*v->z - qz*v->y);
    float ty = 2.0f*(qz*v->x - qx*v->z);
    float tz = 2.0f*(qx*v->y - qy*v->x);

    v->x = v->x + qw*tx + qy*tz - qz*ty;
    v->y = v->y + qw*ty + qz*tx - qx*tz;
    v->z = v->z + qw*tz + qx*ty - qy*tx;

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getRotated(angle) → new Vector, does not mutate ↓ */
static int vecGetRotated(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    struct VanirAng *a;

    if (luaL_testudata(L, 2, "vanir.Angle")) {
        a = (struct VanirAng *)lua_touserdata(L, 2);
    } else {
        luaL_checktype(L, 2, LUA_TTABLE);

        static struct VanirAng temp;
        temp.p = (float)getField(L, 2, "p");
        temp.y = (float)getField(L, 2, "y");
        temp.r = (float)getField(L, 2, "r");
        a = &temp;
    }

    /* ↓ build quaternion from Angle (ZYX: yaw(Z), pitch(Y), roll(X)) ↓ */
    float qw, qx, qy, qz;
    angToQuat(a, &qw, &qx, &qy, &qz);

    float tx = 2.0f*(qy*v->z - qz*v->y);
    float ty = 2.0f*(qz*v->x - qx*v->z);
    float tz = 2.0f*(qx*v->y - qy*v->x);

    pushVec(L,
        v->x + qw*tx + qy*tz - qz*ty,
        v->y + qw*ty + qz*tx - qx*tz,
        v->z + qw*tz + qx*ty - qy*tx
    );

    return 1;
}

/* ↓ :rotateAroundAxis(axis, degrees) → new Vector ↓ */
static int vecRotateAroundAxis(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    struct VanirVec *axis = checkVec(L, 2);
    float rad = degToRad((float)luaL_checknumber(L, 3));

    float c = cosf(rad), s = sinf(rad);
    float dot = axis->x*v->x + axis->y*v->y + axis->z*v->z;

    pushVec(L,
        v->x*c + (axis->y*v->z - axis->z*v->y)*s + axis->x*dot*(1.0f - c),
        v->y*c + (axis->z*v->x - axis->x*v->z)*s + axis->y*dot*(1.0f - c),
        v->z*c + (axis->x*v->y - axis->y*v->x)*s + axis->z*dot*(1.0f - c)
    );

    return 1;
}

/* ↓ :getBasis() → forward, right, up (three Vectors) ↓ */
static int vecGetBasis(lua_State *L) {
    struct VanirVec *fwd = checkVec(L, 1);
    float fx = fwd->x, fy = fwd->y, fz = fwd->z;
    float flen = sqrtf(fx*fx + fy*fy + fz*fz);
    /* ↑ treat self as a forward direction, compute right and up ↑ */

    if (flen > 0.0f) { fx /= flen; fy /= flen; fz /= flen; }

    /* ↓ pick a consistent up reference ↓ */
    float refx = 0.0f, refy = 0.0f, refz = 1.0f;

    if (fabsf(fx) < 0.001f && fabsf(fy) < 0.001f) { refx = 1.0f; refz = 0.0f; }

    float rx = fy*refz - fz*refy;
    float ry = fz*refx - fx*refz;
    float rz = fx*refy - fy*refx;

    float rlen = sqrtf(rx*rx + ry*ry + rz*rz);

    if (rlen > 0.0f) { rx /= rlen; ry /= rlen; rz /= rlen; }

    float ux = ry*fz - rz*fy;
    float uy = rz*fx - rx*fz;
    float uz = rx*fy - ry*fx;

    pushVec(L, fx, fy, fz);
    pushVec(L, rx, ry, rz);
    pushVec(L, ux, uy, uz);

    return 3;
}

/* ↓ :withinAABox(min_vec, max_vec) → bool ↓ */
static int vecWithinAABox(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    struct VanirVec *mn = checkVec(L, 2);
    struct VanirVec *mx = checkVec(L, 3);

    lua_pushboolean(L,
        v->x >= mn->x && v->x <= mx->x &&
        v->y >= mn->y && v->y <= mx->y &&
        v->z >= mn->z && v->z <= mx->z
    );

    return 1;
}

/* ↓ :toScreen() — projects 3D world pos to 2D screen; stub returning table ↓ */
/* ↓ real implementation would need a camera/projection matrix              ↓ */
static int vecToScreen(lua_State *L) {
    /* ↓ without access to the active camera matrices here, return a       ↓ */
    /* ↓ table {x=0, y=0, visible=false} as a safe default stub           ↓ */
    lua_newtable(L);
    setFieldNumber(L, "x", 0.0f);
    setFieldNumber(L, "y", 0.0f);
    lua_pushboolean(L, 0);
    lua_setfield(L, -2, "visible");

    return 1;
}

/* ↓ __index — expose x/y/z fields from userdata to Lua ↓ */
static int vecIndex(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    const char *key = luaL_checkstring(L, 2);

    if (key[1] == '\0') {
        if (key[0] == 'x') { lua_pushnumber(L, v->x); return 1; }
        if (key[0] == 'y') { lua_pushnumber(L, v->y); return 1; }
        if (key[0] == 'z') { lua_pushnumber(L, v->z); return 1; }
    }

    /* ↓ fall through to method table ↓ */
    indexFallback(L, "vanir.Vector", key);

    return 1;
}

/* ↓ __newindex — allow v.x = n style assignment ↓ */
static int vecNewIndex(lua_State *L) {
    struct VanirVec *v = checkVec(L, 1);
    const char *key = luaL_checkstring(L, 2);
    float val = (float)luaL_checknumber(L, 3);

    if (key[1] == '\0') {
        if (key[0] == 'x') { v->x = val; return 0; }
        if (key[0] == 'y') { v->y = val; return 0; }
        if (key[0] == 'z') { v->z = val; return 0; }
    }

    return luaL_error(L, "Vector has no field '%s'", key);
}

static const luaL_Reg vecMethods[] = {
    {"getAngle",         vecGetAngle},
    {"getAngleEx",       vecGetAngleEx},
    {"cross",            vecCross},
    {"getDistance",      vecGetDistance},
    {"getDistanceSqr",   vecGetDistanceSqr},
    {"dot",              vecDot},
    {"getNormalized",    vecGetNormalized},
    {"isEqualTol",       vecIsEqualTol},
    {"isZero",           vecIsZero},
    {"getLength",        vecGetLength},
    {"getLengthSqr",     vecGetLengthSqr},
    {"getLength2D",      vecGetLength2D},
    {"getLength2DSqr",   vecGetLength2DSqr},
    {"add",              vecAddInPlace},
    {"sub",              vecSubInPlace},
    {"mul",              vecMulInPlace},
    {"div",              vecDivInPlace},
    {"vmul",             vecVmul},
    {"vdiv",             vecVdiv},
    {"setZero",          vecSetZero},
    {"setX",             vecSetX},
    {"setY",             vecSetY},
    {"setZ",             vecSetZ},
    {"set",              vecSet},
    {"normalize",        vecNormalize},
    {"rotate",           vecRotate},
    {"getRotated",       vecGetRotated},
    {"rotateAroundAxis", vecRotateAroundAxis},
    {"getBasis",         vecGetBasis},
    {"round",            vecRound},
    {"clone",            vecClone},
    {"withinAABox",      vecWithinAABox},
    {"toScreen",         vecToScreen},

    /* ↓ legacy alias ↓ */
    {"length",           vecGetLength},

    {NULL, NULL}
};

static const luaL_Reg vecMeta[] = {
    {"__tostring", toStringVec},
    {"__add",      vecAdd},
    {"__sub",      vecSub},
    {"__mul",      vecMul},
    {"__div",      vecDiv},
    {"__unm",      vecUnm},
    {"__eq",       vecEq},
    {"__len",      vecLen},
    {"__index",    vecIndex},
    {"__newindex", vecNewIndex},

    {NULL, NULL}
};

/* ↓ Vector(x, y, z) ↓ */
int Vector(lua_State *L) {
    float x = (float)luaL_optnumber(L, 1, 0.0);
    float y = (float)luaL_optnumber(L, 2, 0.0);
    float z = (float)luaL_optnumber(L, 3, 0.0);

    pushVec(L, x, y, z);

    return 1;
}