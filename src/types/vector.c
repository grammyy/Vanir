#include "common.h"

#include <math.h>

static const luaL_Reg vecMethods[];
static const luaL_Reg vecMeta[];

static void pushVec(lua_State *L, float x, float y, float z) {
    lua_newtable(L);
    setFieldNumber(L, "x", x);
    setFieldNumber(L, "y", y);
    setFieldNumber(L, "z", z);
    addMethods(L, "vanir.Vector", vecMethods, vecMeta);
}

/* ↓ __tostring ↓ */
static int toStringVec(lua_State *L) {
    float x = getfieldf(L, 1, "x");
    float y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z");

    lua_pushfstring(L, "(%f, %f, %f)", x, y, z);

    return 1;
}

/* ↓ __add ↓ */
static int vecAdd(lua_State *L) {
    pushVec(L,
        getfieldf(L, 1, "x") + getfieldf(L, 2, "x"),
        getfieldf(L, 1, "y") + getfieldf(L, 2, "y"),
        getfieldf(L, 1, "z") + getfieldf(L, 2, "z")
    );

    return 1;
}

/* ↓ __sub ↓ */
static int vecSub(lua_State *L) {
    pushVec(L,
        getfieldf(L, 1, "x") - getfieldf(L, 2, "x"),
        getfieldf(L, 1, "y") - getfieldf(L, 2, "y"),
        getfieldf(L, 1, "z") - getfieldf(L, 2, "z")
    );

    return 1;
}

/* ↓ __mul: vec * scalar  or  scalar * vec ↓ */
static int vecMul(lua_State *L) {
    float x, y, z, s;

    if (lua_isnumber(L, 1)) {
        s = (float)lua_tonumber(L, 1);
        x = getfieldf(L, 2, "x"); y = getfieldf(L, 2, "y"); z = getfieldf(L, 2, "z");
    } else {
        x = getfieldf(L, 1, "x"); y = getfieldf(L, 1, "y"); z = getfieldf(L, 1, "z");
        s = (float)luaL_checknumber(L, 2);
    }

    pushVec(L, x * s, y * s, z * s);

    return 1;
}

/* ↓ __div: vec / scalar ↓ */
static int vecDiv(lua_State *L) {
    float x = getfieldf(L, 1, "x");
    float y = getfieldf(L, 1, "y");
    float z = getfieldf(L, 1, "z");
    float s = (float)luaL_checknumber(L, 2);

    pushVec(L, x / s, y / s, z / s);

    return 1;
}

/* ↓ __unm ↓ */
static int vecUnm(lua_State *L) {
    pushVec(L, -getfieldf(L, 1, "x"), -getfieldf(L, 1, "y"), -getfieldf(L, 1, "z"));

    return 1;
}

/* ↓ __eq ↓ */
static int vecEq(lua_State *L) {
    lua_pushboolean(L,
        getfieldf(L, 1, "x") == getfieldf(L, 2, "x") &&
        getfieldf(L, 1, "y") == getfieldf(L, 2, "y") &&
        getfieldf(L, 1, "z") == getfieldf(L, 2, "z")
    );

    return 1;
}

/* ↓ __len: #vec → magnitude ↓ */
static int vecLen(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");

    lua_pushnumber(L, sqrtf(x*x + y*y + z*z));

    return 1;
}

/* ↓ :dot(other) ↓ */
static int vecDot(lua_State *L) {
    lua_pushnumber(L,
        getfieldf(L, 1, "x") * getfieldf(L, 2, "x") +
        getfieldf(L, 1, "y") * getfieldf(L, 2, "y") +
        getfieldf(L, 1, "z") * getfieldf(L, 2, "z")
    );

    return 1;
}

/* ↓ :cross(other) → Vector ↓ */
static int vecCross(lua_State *L) {
    float ax = getfieldf(L, 1, "x"), ay = getfieldf(L, 1, "y"), az = getfieldf(L, 1, "z");
    float bx = getfieldf(L, 2, "x"), by = getfieldf(L, 2, "y"), bz = getfieldf(L, 2, "z");

    pushVec(L,
        ay*bz - az*by,
        az*bx - ax*bz,
        ax*by - ay*bx
    );

    return 1;
}

/* ↓ :getLength() ↓ */
static int vecGetLength(lua_State *L) {
    return vecLen(L);
}

/* ↓ :getLengthSqr() ↓ */
static int vecGetLengthSqr(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");

    lua_pushnumber(L, x*x + y*y + z*z);

    return 1;
}

/* ↓ :getLength2D() ↓ */
static int vecGetLength2D(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");

    lua_pushnumber(L, sqrtf(x*x + y*y));

    return 1;
}

/* ↓ :getLength2DSqr() ↓ */
static int vecGetLength2DSqr(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y");

    lua_pushnumber(L, x*x + y*y);

    return 1;
}

/* ↓ :getDistance(other) ↓ */
static int vecGetDistance(lua_State *L) {
    float dx = getfieldf(L, 1, "x") - getfieldf(L, 2, "x");
    float dy = getfieldf(L, 1, "y") - getfieldf(L, 2, "y");
    float dz = getfieldf(L, 1, "z") - getfieldf(L, 2, "z");

    lua_pushnumber(L, sqrtf(dx*dx + dy*dy + dz*dz));

    return 1;
}

/* ↓ :getDistanceSqr(other) ↓ */
static int vecGetDistanceSqr(lua_State *L) {
    float dx = getfieldf(L, 1, "x") - getfieldf(L, 2, "x");
    float dy = getfieldf(L, 1, "y") - getfieldf(L, 2, "y");
    float dz = getfieldf(L, 1, "z") - getfieldf(L, 2, "z");

    lua_pushnumber(L, dx*dx + dy*dy + dz*dz);

    return 1;
}

/* ↓ :getNormalized() → new Vector ↓ */
static int vecGetNormalized(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");
    float len = sqrtf(x*x + y*y + z*z);

    if (len > 0.0f) 
        pushVec(L, x / len, y / len, z / len);
    else            
        pushVec(L, 0.0f, 0.0f, 0.0f);

    return 1;
}

/* ↓ :normalize() — mutates in-place, returns self ↓ */
static int vecNormalize(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");
    float len = sqrtf(x*x + y*y + z*z);

    if (len > 0.0f) {
        setFieldNumber(L, "x", x / len);
        setFieldNumber(L, "y", y / len);
        setFieldNumber(L, "z", z / len);
    } else {
        setFieldNumber(L, "x", 0.0f);
        setFieldNumber(L, "y", 0.0f);
        setFieldNumber(L, "z", 0.0f);
    }

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :isZero([tolerance]) ↓ */
static int vecIsZero(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");
    
    if (!lua_isnoneornil(L, 2)) {
        float tol = (float)lua_tonumber(L, 2);

        lua_pushboolean(L, fabsf(x) <= tol && fabsf(y) <= tol && fabsf(z) <= tol);
    } else {
        lua_pushboolean(L, x == 0.0f && y == 0.0f && z == 0.0f);
    }

    return 1;
}

/* ↓ :isEqualTol(other, tol) ↓ */
static int vecIsEqualTol(lua_State *L) {
    float tol = (float)luaL_checknumber(L, 3);

    lua_pushboolean(L,
        fabsf(getfieldf(L, 1, "x") - getfieldf(L, 2, "x")) <= tol &&
        fabsf(getfieldf(L, 1, "y") - getfieldf(L, 2, "y")) <= tol &&
        fabsf(getfieldf(L, 1, "z") - getfieldf(L, 2, "z")) <= tol
    );

    return 1;
}

/* ↓ :getAngle() → Angle (pitch/yaw from direction) ↓ */
static int vecGetAngle(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");

    float pitch = radToDeg(-atan2f(z, sqrtf(x*x + y*y)));
    float yaw = radToDeg(atan2f(y, x));

    lua_newtable(L);
    setFieldNumber(L, "p", pitch);
    setFieldNumber(L, "y", yaw);
    setFieldNumber(L, "r", 0.0f);

    luaL_getmetatable(L, "vanir.Angle");

    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getAngleEx(up_vec) → Angle with roll derived from up reference ↓ */
static int vecGetAngleEx(lua_State *L) {
    float fx = getfieldf(L, 1, "x"), fy = getfieldf(L, 1, "y"), fz = getfieldf(L, 1, "z");
    float ux = getfieldf(L, 2, "x"), uy = getfieldf(L, 2, "y"), uz = getfieldf(L, 2, "z");

    float flen = sqrtf(fx*fx + fy*fy + fz*fz);

    if (flen > 0.0f) { 
        fx /= flen; 
        fy /= flen; 
        fz /= flen; 
    }

    float pitch = radToDeg(-atan2f(fz, sqrtf(fx*fx + fy*fy)));
    float yaw = radToDeg(atan2f(fy, fx));

    /* ↓ compute right = forward x up, then actual up = right x forward ↓ */
    float rx = fy*uz - fz*uy;
    float ry = fz*ux - fx*uz;
    float rz = fx*uy - fy*ux;

    float rlen = sqrtf(rx*rx + ry*ry + rz*rz);

    if (rlen > 0.0f) { 
        rx /= rlen; 
        ry /= rlen; 
        rz /= rlen; 
    }

    float aupx = ry*fz - rz*fy;
    float aupy = rz*fx - rx*fz;
    float aupz = rx*fy - ry*fx;

    float roll = radToDeg(atan2f(-rx*aupz + rz*aupx, aupy));

    lua_newtable(L);
    setFieldNumber(L, "p", pitch);
    setFieldNumber(L, "y", yaw);
    setFieldNumber(L, "r", roll);

    luaL_getmetatable(L, "vanir.Angle");

    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :add(other) — mutates in-place, returns self ↓ */
static int vecAddInPlace(lua_State *L) {
    setFieldNumber(L, "x", getfieldf(L, 1, "x") + getfieldf(L, 2, "x"));
    setFieldNumber(L, "y", getfieldf(L, 1, "y") + getfieldf(L, 2, "y"));
    setFieldNumber(L, "z", getfieldf(L, 1, "z") + getfieldf(L, 2, "z"));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :sub(other) — mutates in-place, returns self ↓ */
static int vecSubInPlace(lua_State *L) {
    setFieldNumber(L, "x", getfieldf(L, 1, "x") - getfieldf(L, 2, "x"));
    setFieldNumber(L, "y", getfieldf(L, 1, "y") - getfieldf(L, 2, "y"));
    setFieldNumber(L, "z", getfieldf(L, 1, "z") - getfieldf(L, 2, "z"));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :mul(scalar) — mutates in-place, returns self ↓ */
static int vecMulInPlace(lua_State *L) {
    float s = (float)luaL_checknumber(L, 2);

    setFieldNumber(L, "x", getfieldf(L, 1, "x") * s);
    setFieldNumber(L, "y", getfieldf(L, 1, "y") * s);
    setFieldNumber(L, "z", getfieldf(L, 1, "z") * s);
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :div(scalar) — mutates in-place, returns self ↓ */
static int vecDivInPlace(lua_State *L) {
    float s = (float)luaL_checknumber(L, 2);

    setFieldNumber(L, "x", getfieldf(L, 1, "x") / s);
    setFieldNumber(L, "y", getfieldf(L, 1, "y") / s);
    setFieldNumber(L, "z", getfieldf(L, 1, "z") / s);
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :vmul(other_vec) — component-wise multiply, returns new Vector ↓ */
static int vecVmul(lua_State *L) {
    pushVec(L,
        getfieldf(L, 1, "x") * getfieldf(L, 2, "x"),
        getfieldf(L, 1, "y") * getfieldf(L, 2, "y"),
        getfieldf(L, 1, "z") * getfieldf(L, 2, "z")
    );

    return 1;
}

/* ↓ :vdiv(other_vec) — component-wise divide, returns new Vector ↓ */
static int vecVdiv(lua_State *L) {
    pushVec(L,
        getfieldf(L, 1, "x") / getfieldf(L, 2, "x"),
        getfieldf(L, 1, "y") / getfieldf(L, 2, "y"),
        getfieldf(L, 1, "z") / getfieldf(L, 2, "z")
    );

    return 1;
}

/* ↓ :setZero() — mutates in-place, returns self ↓ */
static int vecSetZero(lua_State *L) {
    setFieldNumber(L, "x", 0.0f);
    setFieldNumber(L, "y", 0.0f);
    setFieldNumber(L, "z", 0.0f);
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setX(v) ↓ */
static int vecSetX(lua_State *L) {
    setFieldNumber(L, "x", (float)luaL_checknumber(L, 2));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setY(v) ↓ */
static int vecSetY(lua_State *L) {
    setFieldNumber(L, "y", (float)luaL_checknumber(L, 2));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setZ(v) ↓ */
static int vecSetZ(lua_State *L) {
    setFieldNumber(L, "z", (float)luaL_checknumber(L, 2));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :set(x, y, z) — mutates in-place, returns self ↓ */
static int vecSet(lua_State *L) {
    setFieldNumber(L, "x", (float)luaL_checknumber(L, 2));
    setFieldNumber(L, "y", (float)luaL_checknumber(L, 3));
    setFieldNumber(L, "z", (float)luaL_checknumber(L, 4));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :clone() ↓ */
static int vecClone(lua_State *L) {
    pushVec(L, getfieldf(L, 1, "x"), getfieldf(L, 1, "y"), getfieldf(L, 1, "z"));

    return 1;
}

/* ↓ :round([decimals]) → new Vector ↓ */
static int vecRound(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");
    float mul = 1.0f;

    if (!lua_isnoneornil(L, 2)) {
        int dec = (int)lua_tointeger(L, 2);

        for (int i = 0; i < dec; i++) 
            mul *= 10.0f;
    }

    pushVec(L,
        roundf(x * mul) / mul,
        roundf(y * mul) / mul,
        roundf(z * mul) / mul
    );

    return 1;
}

/* ↓ :rotate(angle) — rotates vector in-place by pitch/yaw/roll of Angle ↓ */
static int vecRotate(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");
    float pitch = degToRad(getfieldf(L, 2, "p"));
    float yaw = degToRad(getfieldf(L, 2, "y"));
    float roll = degToRad(getfieldf(L, 2, "r"));

    /* ↓ apply yaw (Z), pitch (Y), roll (X) ↓ */
    float cy = cosf(yaw), sy = sinf(yaw);
    float cp = cosf(pitch), sp = sinf(pitch);
    float cr = cosf(roll),  sr = sinf(roll);

    float rx = (cy*cp)*x + (cy*sp*sr - sy*cr)*y + (cy*sp*cr + sy*sr)*z;
    float ry = (sy*cp)*x + (sy*sp*sr + cy*cr)*y + (sy*sp*cr - cy*sr)*z;
    float rz = (-sp)*x   + (cp*sr)*y            + (cp*cr)*z;

    setFieldNumber(L, "x", rx);
    setFieldNumber(L, "y", ry);
    setFieldNumber(L, "z", rz);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :getRotated(angle) → new Vector, does not mutate ↓ */
static int vecGetRotated(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");
    float pitch = degToRad(getfieldf(L, 2, "p"));
    float yaw = degToRad(getfieldf(L, 2, "y"));
    float roll = degToRad(getfieldf(L, 2, "r"));

    float cy = cosf(yaw), sy = sinf(yaw);
    float cp = cosf(pitch), sp = sinf(pitch);
    float cr = cosf(roll), sr = sinf(roll);

    pushVec(L,
        (cy*cp)*x + (cy*sp*sr - sy*cr)*y + (cy*sp*cr + sy*sr)*z,
        (sy*cp)*x + (sy*sp*sr + cy*cr)*y + (sy*sp*cr - cy*sr)*z,
        (-sp)*x   + (cp*sr)*y            + (cp*cr)*z
    );

    return 1;
}

/* ↓ :rotateAroundAxis(axis, degrees) → new Vector ↓ */
static int vecRotateAroundAxis(lua_State *L) {
    float vx = getfieldf(L, 1, "x"), vy = getfieldf(L, 1, "y"), vz = getfieldf(L, 1, "z");
    float ax = getfieldf(L, 2, "x"), ay = getfieldf(L, 2, "y"), az = getfieldf(L, 2, "z");
    float rad = degToRad((float)luaL_checknumber(L, 3));

    float c = cosf(rad), s = sinf(rad);
    float dot = ax*vx + ay*vy + az*vz;

    pushVec(L,
        vx*c + (ay*vz - az*vy)*s + ax*dot*(1.0f - c),
        vy*c + (az*vx - ax*vz)*s + ay*dot*(1.0f - c),
        vz*c + (ax*vy - ay*vx)*s + az*dot*(1.0f - c)
    );

    return 1;
}

/* ↓ :getBasis() → forward, right, up (three Vectors) ↓ */
static int vecGetBasis(lua_State *L) {
    float fx = getfieldf(L, 1, "x"), fy = getfieldf(L, 1, "y"), fz = getfieldf(L, 1, "z");
    float flen = sqrtf(fx*fx + fy*fy + fz*fz);
    /* ↑ treat self as a forward direction, compute right and up ↑ */
    
    if (flen > 0.0f) { 
        fx /= flen; 
        fy /= flen; 
        fz /= flen; 
    }

    /* ↓ pick a consistent up reference ↓ */
    float refx = 0.0f, refy = 0.0f, refz = 1.0f;
    
    if (fabsf(fx) < 0.001f && fabsf(fy) < 0.001f) { 
        refx = 1.0f; 
        refz = 0.0f; 
    }

    float rx = fy*refz - fz*refy;
    float ry = fz*refx - fx*refz;
    float rz = fx*refy - fy*refx;

    float rlen = sqrtf(rx*rx + ry*ry + rz*rz);
    
    if (rlen > 0.0f) { 
        rx /= rlen; 
        ry /= rlen; 
        rz /= rlen; 
    }

    float ux = ry*fz - rz*fy;
    float uy = rz*fx - rx*fz;
    float uz = rx*fy - ry*fx;

    pushVec(L, fx, fy, fz);
    pushVec(L, rx, ry, rz);
    pushVec(L, ux, uy, uz);

    return 3;
}

/* ↓ :getColor() → Color(x,y,z,255) ↓ */
static int vecGetColor(lua_State *L) {
    lua_newtable(L);
    setFieldNumber(L, "r", getfieldf(L, 1, "x"));
    setFieldNumber(L, "g", getfieldf(L, 1, "y"));
    setFieldNumber(L, "b", getfieldf(L, 1, "z"));
    setFieldNumber(L, "a", 255.0f);

    luaL_getmetatable(L, "vanir.Color");

    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :withinAABox(min_vec, max_vec) → bool ↓ */
static int vecWithinAABox(lua_State *L) {
    float x = getfieldf(L, 1, "x"), y = getfieldf(L, 1, "y"), z = getfieldf(L, 1, "z");
    float minx = getfieldf(L, 2, "x"), miny = getfieldf(L, 2, "y"), minz = getfieldf(L, 2, "z");
    float maxx = getfieldf(L, 3, "x"), maxy = getfieldf(L, 3, "y"), maxz = getfieldf(L, 3, "z");

    lua_pushboolean(L,
        x >= minx && x <= maxx &&
        y >= miny && y <= maxy &&
        z >= minz && z <= maxz
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
    {"getColor",         vecGetColor},
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