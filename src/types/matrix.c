#include "common.h"

#include <math.h>
#include <string.h>

/* ↓ 3×3 affine matrix, stored row-major in a 9-element flat Lua table (1-indexed) ↓ */
/* ↓ layout:                                                                        ↓ */
/*   | m[1]  m[2]  m[3] |   [ rx  ux  fx ]   (right, up, forward columns)          */
/*   | m[4]  m[5]  m[6] |   [ ry  uy  fy ]                                         */
/*   | m[7]  m[8]  m[9] |   [ rz  uz  fz ]   (or tx ty 1 for 2-D affine)           */
/*                                                                                   */
/* ↓ 2-D affine convention (used by render pipeline):                              ↓ */
/*   m[0..1] = cosθ/-sinθ,  m[2] = tx                                              */
/*   m[3..4] = sinθ/ cosθ,  m[5] = ty                                              */
/*   m[6]=0  m[7]=0  m[8]=1                                                        */
/*                                                                                   */
/* ↓ mutation semantics: in-place mutators return self for chaining                ↓ */

static const luaL_Reg matMethods[];
static const luaL_Reg matMeta[];

/* ↓ pull all 9 elements off a table at stack index idx ↓ */
static void matGet(lua_State *L, int idx, float m[9]) {
    for (int i = 0; i < 9; i++) {
        lua_rawgeti(L, idx, i + 1);
        
        m[i] = (float)lua_tonumber(L, -1);
        
        lua_pop(L, 1);
    }
}

/* ↓ write 9 elements back into the table at stack index idx ↓ */
static void matSet(lua_State *L, int idx, const float m[9]) {
    if (idx < 0) 
        idx = lua_gettop(L) + idx + 1;

    for (int i = 0; i < 9; i++) {
        lua_pushnumber(L, m[i]);
        lua_rawseti(L, idx, i + 1);
    }
}

/* ↓ push a brand-new matrix table with metatable ↓ */
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

/* ↓ column-vector helpers: columns 0/1/2 → right/up/forward ↓ */
static void matGetCol(const float m[9], int col, float *x, float *y, float *z) {
    *x = m[col];
    *y = m[3 + col];
    *z = m[6 + col];
}

static void matSetCol(float m[9], int col, float x, float y, float z) {
    m[col] = x;
    m[3 + col] = y;
    m[6 + col] = z;
}

/* ↓ __tostring ↓ */
static int matToString(lua_State *L) {
    float m[9];
    char buf[256];

    matGet(L, 1, m);

    snprintf(buf, sizeof(buf),
        "| %.3f %.3f %.3f | %.3f %.3f %.3f | %.3f %.3f %.3f |",
        m[0], m[1], m[2],
        m[3], m[4], m[5],
        m[6], m[7], m[8]);

    lua_pushstring(L, buf);

    return 1;
}

/* ↓ __add: element-wise addition ↓ */
static int matAdd(lua_State *L) {
    float a[9], b[9], out[9];
    
    matGet(L, 1, a);
    matGet(L, 2, b);

    for (int i = 0; i < 9; i++) 
        out[i] = a[i] + b[i];

    matPush(L, out);

    return 1;
}

/* ↓ __sub: element-wise subtraction ↓ */
static int matSub(lua_State *L) {
    float a[9], b[9], out[9];
    
    matGet(L, 1, a);
    matGet(L, 2, b);

    for (int i = 0; i < 9; i++) 
        out[i] = a[i] - b[i];

    matPush(L, out);

    return 1;
}

/* ↓ __mul: Matrix * Matrix  or  Matrix * Vector ↓ */
static int matMul(lua_State *L) {
    float a[9];
    
    matGet(L, 1, a);

    struct VanirVec *v = checkVec(L, 2);

    if (v) {
        float vx = v->x;
        float vy = v->y;

        struct VanirVec *out = (struct VanirVec *)lua_newuserdata(L, sizeof(struct VanirVec));
        
        out->x = a[0]*vx + a[1]*vy + a[2];
        out->y = a[3]*vx + a[4]*vy + a[5];
        out->z = 0.0f;

        luaL_setmetatable(L, "vanir.Vector");

        return 1;
    }
    
    float b[9], out[9];

    matGet(L, 2, b);
    mat3Mul(a, b, out);
    matPush(L, out);

    return 1;
}

/* read methods ↓↓↓ read methods */
/* ↓ :getAngles() → Angle (pitch/yaw/roll extracted from rotation columns) ↓ */
static int matGetAngles(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    /* ↓ forward column = col 2 ↓ */
    float fx = m[2], fy = m[5], fz = m[8];
    /* ↓ right column = col 0 ↓ */
    float rx = m[0], ry = m[3], rz = m[6];

    float pitch = radToDeg(-asinf(fz));
    float yaw = radToDeg(atan2f(fy, fx));
    float roll = radToDeg(atan2f(-rz, sqrtf(rx*rx + ry*ry)));

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

/* ↓ :getScale() → Vector of column norms ↓ */
static int matGetScale(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float sx = sqrtf(m[0]*m[0] + m[3]*m[3] + m[6]*m[6]);
    float sy = sqrtf(m[1]*m[1] + m[4]*m[4] + m[7]*m[7]);
    float sz = sqrtf(m[2]*m[2] + m[5]*m[5] + m[8]*m[8]);

    lua_newtable(L);
    setFieldNumber(L, "x", sx);
    setFieldNumber(L, "y", sy);
    setFieldNumber(L, "z", sz);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getTranslation() → Vector(m[2], m[5], 0) ↓ */
static int matGetTranslation(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    lua_newtable(L);
    setFieldNumber(L, "x", m[2]);
    setFieldNumber(L, "y", m[5]);
    setFieldNumber(L, "z", m[8]);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getField(row, col) → number (1-indexed) ↓ */
static int matGetField(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    int row = (int)luaL_checkinteger(L, 2) - 1;
    int col = (int)luaL_checkinteger(L, 3) - 1;

    if (row < 0 || row > 2 || col < 0 || col > 2)
        throw("error", "Matrix:getField", "row/col out of range [1,3]");

    lua_pushnumber(L, m[row * 3 + col]);

    return 1;
}

/* ↓ :getForward() → Vector (column 2) ↓ */
static int matGetForward(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    lua_newtable(L);
    setFieldNumber(L, "x", m[2]);
    setFieldNumber(L, "y", m[5]);
    setFieldNumber(L, "z", m[8]);

    luaL_getmetatable(L, "vanir.Vector");

    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getRight() → Vector (column 0) ↓ */
static int matGetRight(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    lua_newtable(L);
    setFieldNumber(L, "x", m[0]);
    setFieldNumber(L, "y", m[3]);
    setFieldNumber(L, "z", m[6]);

    luaL_getmetatable(L, "vanir.Vector");
    
    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getUp() → Vector (column 1) ↓ */
static int matGetUp(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    lua_newtable(L);
    setFieldNumber(L, "x", m[1]);
    setFieldNumber(L, "y", m[4]);
    setFieldNumber(L, "z", m[7]);

    luaL_getmetatable(L, "vanir.Vector");
    
    if (!lua_isnil(L, -1)) 
        lua_setmetatable(L, -2);
    else 
        lua_pop(L, 1);

    return 1;
}

/* ↓ :getInverse() — returns new matrix, general 3×3 inverse ↓ */
static int matGetInverse(lua_State *L) {
    float m[9];
    matGet(L, 1, m);

    float det =
        m[0]*(m[4]*m[8] - m[5]*m[7]) -
        m[1]*(m[3]*m[8] - m[5]*m[6]) +
        m[2]*(m[3]*m[7] - m[4]*m[6]);

    float inv[9];

    if (fabsf(det) < 1e-9f) {
        vanir_log("Matrix:getInverse: singular matrix, returning identity");
        mat3Identity(inv);
    } else {
        float invDet = 1.0f / det;

        inv[0] =  (m[4]*m[8] - m[5]*m[7]) * invDet;
        inv[1] = -(m[1]*m[8] - m[2]*m[7]) * invDet;
        inv[2] =  (m[1]*m[5] - m[2]*m[4]) * invDet;

        inv[3] = -(m[3]*m[8] - m[5]*m[6]) * invDet;
        inv[4] =  (m[0]*m[8] - m[2]*m[6]) * invDet;
        inv[5] = -(m[0]*m[5] - m[2]*m[3]) * invDet;

        inv[6] =  (m[3]*m[7] - m[4]*m[6]) * invDet;
        inv[7] = -(m[0]*m[7] - m[1]*m[6]) * invDet;
        inv[8] =  (m[0]*m[4] - m[1]*m[3]) * invDet;
    }

    matPush(L, inv);

    return 1;
}

/* ↓ :getInverseTR() — fast inverse for orthogonal rotation + translation matrix ↓ */
/* ↓ inv(R|t) = R^T | -R^T*t                                                    ↓ */
static int matGetInverseTR(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float inv[9];

    /* ↓ transpose the 2×2 rotation part ↓ */
    inv[0] = m[0]; inv[1] = m[3]; inv[3] = m[1]; inv[4] = m[4];
    inv[6] = 0.0f; inv[7] = 0.0f; inv[8] = 1.0f;

    /* ↓ -R^T * t ↓ */
    inv[2] = -(inv[0]*m[2] + inv[1]*m[5]);
    inv[5] = -(inv[3]*m[2] + inv[4]*m[5]);

    matPush(L, inv);

    return 1;
}

/* ↓ :getRotatedAroundAxis(axis_vec, degrees) → new Matrix ↓ */
static int matGetRotatedAroundAxis(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float ax = getfieldf(L, 2, "x");
    float ay = getfieldf(L, 2, "y");
    float az = getfieldf(L, 2, "z");
    float rad = degToRad((float)luaL_checknumber(L, 3));

    float c = cosf(rad), s = sinf(rad), t = 1.0f - c;

    float r[9] = {
        t*ax*ax + c,       t*ax*ay - s*az,    t*ax*az + s*ay,
        t*ax*ay + s*az,    t*ay*ay + c,       t*ay*az - s*ax,
        t*ax*az - s*ay,    t*ay*az + s*ax,    t*az*az + c
    };

    float out[9];

    mat3Mul(r, m, out);
    matPush(L, out);

    return 1;
}

/* ↓ :isIdentity() ↓ */
static int matIsIdentity(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    lua_pushboolean(L,
        m[0] == 1.0f && m[1] == 0.0f && m[2] == 0.0f &&
        m[3] == 0.0f && m[4] == 1.0f && m[5] == 0.0f &&
        m[6] == 0.0f && m[7] == 0.0f && m[8] == 1.0f
    );

    return 1;
}

/* ↓ :isRotationMatrix() — checks columns are orthonormal ↓ */
static int matIsRotationMatrix(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    /* ↓ check column norms are ~1 and columns are mutually orthogonal ↓ */
    float c0x = m[0], c0y = m[3], c0z = m[6];
    float c1x = m[1], c1y = m[4], c1z = m[7];
    float c2x = m[2], c2y = m[5], c2z = m[8];

    float n0 = c0x*c0x + c0y*c0y + c0z*c0z;
    float n1 = c1x*c1x + c1y*c1y + c1z*c1z;
    float n2 = c2x*c2x + c2y*c2y + c2z*c2z;
    float d01 = c0x*c1x + c0y*c1y + c0z*c1z;
    float d02 = c0x*c2x + c0y*c2y + c0z*c2z;
    float d12 = c1x*c2x + c1y*c2y + c1z*c2z;
    float eps = 1e-4f;

    lua_pushboolean(L,
        fabsf(n0 - 1.0f) < eps && fabsf(n1 - 1.0f) < eps && fabsf(n2 - 1.0f) < eps &&
        fabsf(d01) < eps && fabsf(d02) < eps && fabsf(d12) < eps
    );

    return 1;
}

/* ↓ :getAxisAngle() → axis_vec, angle_degrees ↓ */
static int matGetAxisAngle(lua_State *L) {
    float m[9];
    
    matGet(L, 1, m);

    float angle = acosf(fmaxf(-1.0f, fminf(1.0f, (m[0] + m[4] + m[8] - 1.0f) * 0.5f)));
    float s = sinf(angle);

    float ax = 0.0f, ay = 0.0f, az = 1.0f;

    if (fabsf(s) > 1e-6f) {
        float inv2s = 0.5f / s;
        ax = (m[7] - m[5]) * inv2s;
        ay = (m[2] - m[6]) * inv2s;
        az = (m[3] - m[1]) * inv2s;
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

    lua_pushnumber(L, radToDeg(angle));

    return 2;
}

/* ↓ :clone() ↓ */
static int matClone(lua_State *L) {
    float m[9];

    matGet(L, 1, m);
    matPush(L, m);

    return 1;
}

/* ↓ :unpack() → 9 numbers ↓ */
static int matUnpack(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    for (int i = 0; i < 9; i++) 
        lua_pushnumber(L, m[i]);

    return 9;
}

/* ↓ :toTable() → plain table {[1]...[9]} without metatable ↓ */
static int matToTable(lua_State *L) {
    float m[9];
    
    matGet(L, 1, m);

    lua_createtable(L, 9, 0);
    
    for (int i = 0; i < 9; i++) {
        lua_pushnumber(L, m[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}
/* read methods ↑↑↑ read methods*/

/* mutating methods ↓↓↓ mutating methods */
/* ↓ :translate(dx, dy [, dz]) — mutates in-place, returns self ↓ */
static int matTranslate(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float dx = (float)luaL_checknumber(L, 2);
    float dy = (float)luaL_optnumber(L, 3, 0.0);

    float t[9] = { 1,0,dx, 0,1,dy, 0,0,1 };
    float out[9];

    mat3Mul(m, t, out);
    matSet(L, 1, out);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :rotate(degrees_or_Angle) — mutates in-place, returns self ↓ */
static int matRotate(lua_State *L) {
    float m[9];
    matGet(L, 1, m);

    float deg;

    if (lua_isnumber(L, 2)) {
        deg = (float)lua_tonumber(L, 2);
    } else {
        lua_getfield(L, 2, "r");

        deg = (float)lua_tonumber(L, -1);

        lua_pop(L, 1);
    }

    float rad = degToRad(deg);
    float c = cosf(rad);
    float s = sinf(rad);

    float r[9] = { c,-s,0, s,c,0, 0,0,1 };
    float out[9];

    mat3Mul(m, r, out);
    matSet(L, 1, out);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :scale(sx [, sy]) — mutates in-place, returns self ↓ */
static int matScale(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float sx = (float)luaL_checknumber(L, 2);
    float sy = (float)luaL_optnumber(L, 3, sx);

    float s[9] = { sx,0,0, 0,sy,0, 0,0,1 };
    float out[9];

    mat3Mul(m, s, out);
    matSet(L, 1, out);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setScale(sx, sy [, sz]) — sets column norms, mutates in-place ↓ */
static int matSetScale(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float sx = (float)luaL_checknumber(L, 2);
    float sy = (float)luaL_optnumber(L, 3, sx);
    float sz = (float)luaL_optnumber(L, 4, sx);

    /* ↓ normalise each column then rescale ↓ */
    float c0len = sqrtf(m[0]*m[0] + m[3]*m[3] + m[6]*m[6]);
    float c1len = sqrtf(m[1]*m[1] + m[4]*m[4] + m[7]*m[7]);
    float c2len = sqrtf(m[2]*m[2] + m[5]*m[5] + m[8]*m[8]);

    if (c0len > 0.0f) { m[0] = m[0]/c0len*sx; m[3] = m[3]/c0len*sx; m[6] = m[6]/c0len*sx; }
    if (c1len > 0.0f) { m[1] = m[1]/c1len*sy; m[4] = m[4]/c1len*sy; m[7] = m[7]/c1len*sy; }
    if (c2len > 0.0f) { m[2] = m[2]/c2len*sz; m[5] = m[5]/c2len*sz; m[8] = m[8]/c2len*sz; }

    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :scaleTranslation(scalar) — scales translation component only ↓ */
static int matScaleTranslation(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float s = (float)luaL_checknumber(L, 2);

    m[2] *= s;
    m[5] *= s;

    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setAngles(p, y, r)  or  :setAngles(Angle) — sets rotation columns ↓ */
static int matSetAngles(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float pitch, yaw, roll;

    if (lua_istable(L, 2)) {
        pitch = getfieldf(L, 2, "p");
        yaw = getfieldf(L, 2, "y");
        roll = getfieldf(L, 2, "r");
    } else {
        pitch = (float)luaL_checknumber(L, 2);
        yaw = (float)luaL_optnumber(L, 3, 0.0);
        roll = (float)luaL_optnumber(L, 4, 0.0);
    }

    float cp = cosf(degToRad(pitch)), sp = sinf(degToRad(pitch));
    float cy = cosf(degToRad(yaw)), sy = sinf(degToRad(yaw));
    float cr = cosf(degToRad(roll)), sr = sinf(degToRad(roll));

    /* ↓ right column (col 0) ↓ */
    m[0] =  cr*cy + sr*sp*sy;
    m[3] =  cr*sy - sr*sp*cy;
    m[6] = -sr*cp;

    /* ↓ up column (col 1) ↓ */
    m[1] = -sr*cy + cr*sp*sy;
    m[4] = -sr*sy - cr*sp*cy;
    m[7] =  cr*cp;

    /* ↓ forward column (col 2) ↓ */
    m[2] = cp*cy;
    m[5] = cp*sy;
    m[8] = -sp;

    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setTranslation(x, y [, z])  or  Vector ↓ */
static int matSetTranslation(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    if (lua_istable(L, 2)) {
        m[2] = getfieldf(L, 2, "x");
        m[5] = getfieldf(L, 2, "y");
        m[8] = getfieldf(L, 2, "z");
    } else {
        m[2] = (float)luaL_checknumber(L, 2);
        m[5] = (float)luaL_optnumber(L, 3, 0.0);
        m[8] = (float)luaL_optnumber(L, 4, 0.0);
    }

    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setForward(vec) — sets column 2 ↓ */
static int matSetForward(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    matSetCol(m, 2, 
        getfieldf(L, 2, "x"), 
        getfieldf(L, 2, "y"), 
        getfieldf(L, 2, "z")
    );

    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setRight(vec) — sets column 0 ↓ */
static int matSetRight(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    matSetCol(m, 0, 
        getfieldf(L, 2, "x"), 
        getfieldf(L, 2, "y"), 
        getfieldf(L, 2, "z")
    );

    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setUp(vec) — sets column 1 ↓ */
static int matSetUp(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    matSetCol(m, 1, 
        getfieldf(L, 2, "x"), 
        getfieldf(L, 2, "y"), 
        getfieldf(L, 2, "z")
    );

    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setField(row, col, value) — 1-indexed ↓ */
static int matSetField(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    int row = (int)luaL_checkinteger(L, 2) - 1;
    int col = (int)luaL_checkinteger(L, 3) - 1;

    if (row < 0 || row > 2 || col < 0 || col > 2)
        throw("error", "Matrix:setField", "row/col out of range [1,3]");

    m[row * 3 + col] = (float)luaL_checknumber(L, 4);

    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :set(other) — copies another matrix into self, mutates in-place ↓ */
static int matSetFrom(lua_State *L) {
    float m[9];

    matGet(L, 2, m);
    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setUnpacked(m1..m9) — set all 9 elements from 9 numbers ↓ */
static int matSetUnpacked(lua_State *L) {
    float m[9];

    for (int i = 0; i < 9; i++)
        m[i] = (float)luaL_checknumber(L, i + 2);

    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setIdentity() — mutates in-place, returns self ↓ */
static int matSetIdentity(lua_State *L) {
    float m[9];

    mat3Identity(m);
    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :invert() — mutates in-place using general inverse, returns self ↓ */
static int matInvert(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float det =
        m[0]*(m[4]*m[8] - m[5]*m[7]) -
        m[1]*(m[3]*m[8] - m[5]*m[6]) +
        m[2]*(m[3]*m[7] - m[4]*m[6]);

    float inv[9];

    if (fabsf(det) < 1e-9f) {
        vanir_log("Matrix:invert: singular matrix, setting identity");
        mat3Identity(inv);
    } else {
        float d = 1.0f / det;
        inv[0] =  (m[4]*m[8] - m[5]*m[7]) * d;
        inv[1] = -(m[1]*m[8] - m[2]*m[7]) * d;
        inv[2] =  (m[1]*m[5] - m[2]*m[4]) * d;
        inv[3] = -(m[3]*m[8] - m[5]*m[6]) * d;
        inv[4] =  (m[0]*m[8] - m[2]*m[6]) * d;
        inv[5] = -(m[0]*m[5] - m[2]*m[3]) * d;
        inv[6] =  (m[3]*m[7] - m[4]*m[6]) * d;
        inv[7] = -(m[0]*m[7] - m[1]*m[6]) * d;
        inv[8] =  (m[0]*m[4] - m[1]*m[3]) * d;
    }

    matSet(L, 1, inv);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :invertTR() — fast transpose inverse for rotation+translation ↓ */
static int matInvertTR(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float inv[9];
    inv[0] = m[0]; inv[1] = m[3]; inv[3] = m[1]; inv[4] = m[4];
    inv[6] = 0.0f; inv[7] = 0.0f; inv[8] = 1.0f;
    inv[2] = -(inv[0]*m[2] + inv[1]*m[5]);
    inv[5] = -(inv[3]*m[2] + inv[4]*m[5]);

    matSet(L, 1, inv);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setAxisAngle(axis_vec, degrees) — builds rotation matrix ↓ */
static int matSetAxisAngle(lua_State *L) {
    float ax = getfieldf(L, 2, "x");
    float ay = getfieldf(L, 2, "y");
    float az = getfieldf(L, 2, "z");
    float rad = degToRad((float)luaL_checknumber(L, 3));

    float c   = cosf(rad), s = sinf(rad), t = 1.0f - c;

    float m[9] = {
        t*ax*ax + c,       t*ax*ay - s*az,    t*ax*az + s*ay,
        t*ax*ay + s*az,    t*ay*ay + c,       t*ay*az - s*ax,
        t*ax*az - s*ay,    t*ay*az + s*ax,    t*az*az + c
    };

    matSet(L, 1, m);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :transformPoint(x, y) → rx, ry — kept for backwards compat ↓ */
static int matTransformPoint(lua_State *L) {
    float m[9];

    matGet(L, 1, m);

    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);

    lua_pushnumber(L, m[0]*x + m[1]*y + m[2]);
    lua_pushnumber(L, m[3]*x + m[4]*y + m[5]);

    return 2;
}
/* mutating methods ↑↑↑ mutating methods */

/* ↓ meta functions ↓ */
static const luaL_Reg matMethods[] = {
    /* ↓ read methods ↓ */
    {"getAngles",            matGetAngles},
    {"getScale",             matGetScale},
    {"getTranslation",       matGetTranslation},
    {"getField",             matGetField},
    {"getForward",           matGetForward},
    {"getRight",             matGetRight},
    {"getUp",                matGetUp},
    {"getInverse",           matGetInverse},
    {"getInverseTR",         matGetInverseTR},
    {"getRotatedAroundAxis", matGetRotatedAroundAxis},
    {"getAxisAngle",         matGetAxisAngle},
    {"isIdentity",           matIsIdentity},
    {"isRotationMatrix",     matIsRotationMatrix},
    {"clone",                matClone},
    {"unpack",               matUnpack},
    {"toTable",              matToTable},

    /* ↓ mutating methods ↓ */
    {"rotate",           matRotate},
    {"translate",        matTranslate},
    {"scale",            matScale},
    {"setScale",         matSetScale},
    {"scaleTranslation", matScaleTranslation},
    {"setAngles",        matSetAngles},
    {"setTranslation",   matSetTranslation},
    {"setForward",       matSetForward},
    {"setRight",         matSetRight},
    {"setUp",            matSetUp},
    {"setField",         matSetField},
    {"set",              matSetFrom},
    {"setUnpacked",      matSetUnpacked},
    {"setIdentity",      matSetIdentity},
    {"invert",           matInvert},
    {"invertTR",         matInvertTR},
    {"setAxisAngle",     matSetAxisAngle},

    /* ↓ backwards compat ↓ */
    {"transformPoint",   matTransformPoint},
    {"inverse",          matGetInverse},
    {"copy",             matClone},

    {NULL, NULL}
};

/* ↓ meta tables ↓ */
static const luaL_Reg matMeta[] = {
    {"__tostring", matToString},
    {"__add",      matAdd},
    {"__sub",      matSub},
    {"__mul",      matMul},
    //{__eq ,        matEq},

    {NULL, NULL}
};

/* ↓ Matrix(angle_or_nil, vector_or_nil)                                    ↓ */
/* ↓ no args → identity                                                     ↓ */
/* ↓ first arg = number → rotation by degrees                               ↓ */
/* ↓ first arg = Angle  → rotation by .y (yaw)                              ↓ */
/* ↓ second arg = Vector → translation by .x/.y                             ↓ */
int Matrix(lua_State *L) {
    float m[9];

    mat3Identity(m);

    if (!lua_isnoneornil(L, 1)) {
        float deg;

        if (lua_isnumber(L, 1)) {
            deg = (float)lua_tonumber(L, 1);
        } else {
            lua_getfield(L, 1, "r");

            deg = (float)lua_tonumber(L, -1);

            lua_pop(L, 1);
        }

        float rad = degToRad(deg);
        float c = cosf(rad);
        float s = sinf(rad);

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

/* ↓  public helpers used by render.c ↓ */

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