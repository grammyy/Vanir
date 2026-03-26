/* types.c */
#include "types.h"
#include "vanir.h"
#include "modules/files.h"
#include "graphics/textures.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ↓ internal functions */
static void pushRegList(lua_State *L, const luaL_Reg *reg) {
    for (; reg->name != NULL; ++reg) {
        lua_pushcfunction(L, reg->func);
        lua_setfield(L, -2, reg->name);
    }
}

void addMethods(lua_State *L, const char *name, const luaL_Reg *methods, const luaL_Reg *meta) {
    luaL_newmetatable(L, name);

    if (methods) {
        lua_newtable(L);
        pushRegList(L, methods);
        lua_setfield(L, -2, "__index");
    }

    if (meta)
        pushRegList(L, meta);

    lua_setmetatable(L, -2);
}

static float getfieldf(lua_State *L, int idx, const char *key) {
    lua_getfield(L, idx, key);
    
    float v = (float)lua_tonumber(L, -1);
    
    lua_pop(L, 1);
    
    return v;
}
/* ↑ internal functions ↑ */

/* ↓ vector type ↓ */
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

// ↓ scalar multiply: vec * number  or  number * vec ↓
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

// #vec → magnitude
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
    {"dot", vecDot},
    {"cross", vecCross},
    {"normalize", vecNormalize},
    {"length", vecLength},

    {NULL, NULL}
};

static const luaL_Reg vecMeta[] = {
    {"__tostring", toStringVec },
    {"__add", vecAdd },
    {"__sub", vecSub },
    {"__mul", vecMul },
    {"__unm", vecUnm },
    {"__eq", vecEq },
    {"__len", vecLen },

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

/* ↓ angle type ↓ */
static int toStringAngle(lua_State *L) {
    float roll  = getfieldf(L, 1, "roll");
    float pitch = getfieldf(L, 1, "pitch");
    float yaw   = getfieldf(L, 1, "yaw");
    
    lua_pushfstring(L, "(%f, %f, %f)", roll, pitch, yaw);
    
    return 1;
}

static const luaL_Reg angleMeta[] = {
    { "__tostring", toStringAngle },
    
    { NULL, NULL }
};

int Angle(lua_State *L) {
    float roll = (float)luaL_optnumber(L, 1, 0.0);
    float pitch = (float)luaL_optnumber(L, 2, 0.0);
    float yaw = (float)luaL_optnumber(L, 3, 0.0);

    lua_newtable(L);
    setFieldNumber(L, "roll", roll);
    setFieldNumber(L, "pitch", pitch);
    setFieldNumber(L, "yaw", yaw);

    addMethods(L, "vanir.Angle", NULL, angleMeta);
    
    return 1;
}

//* ↓ color type ↓ */
static const luaL_Reg colorMethods[];
static const luaL_Reg colorMeta[];

static void pushColor(lua_State *L, float r, float g, float b, float a) {
    lua_newtable(L);
    setFieldNumber(L, "r", r);
    setFieldNumber(L, "g", g);
    setFieldNumber(L, "b", b);
    setFieldNumber(L, "a", a);
    addMethods(L, "vanir.Color", colorMethods, colorMeta);
}

static int toStringColor(lua_State *L) {
    float r = getfieldf(L, 1, "r"), g = getfieldf(L, 1, "g");
    float b = getfieldf(L, 1, "b"), a = getfieldf(L, 1, "a");
    
    lua_pushfstring(L, "(%f, %f, %f, %f)", r, g, b, a);
    
    return 1;
}

static int colorEq(lua_State *L) {
    lua_pushboolean(L,
        getfieldf(L, 1, "r") == getfieldf(L, 2, "r") &&
        getfieldf(L, 1, "g") == getfieldf(L, 2, "g") &&
        getfieldf(L, 1, "b") == getfieldf(L, 2, "b") &&
        getfieldf(L, 1, "a") == getfieldf(L, 2, "a"));

    return 1;
}

/* ↓ color:lerp(other, t) linear interpolate between two colors ↓ */
static int colorLerp(lua_State *L) {
    float r1 = getfieldf(L, 1, "r"), g1 = getfieldf(L, 1, "g");
    float b1 = getfieldf(L, 1, "b"), a1 = getfieldf(L, 1, "a");
    float r2 = getfieldf(L, 2, "r"), g2 = getfieldf(L, 2, "g");
    float b2 = getfieldf(L, 2, "b"), a2 = getfieldf(L, 2, "a");
    float t  = (float)luaL_checknumber(L, 3);
    
    pushColor(L,
        r1 + (r2 - r1) * t,
        g1 + (g2 - g1) * t,
        b1 + (b2 - b1) * t,
        a1 + (a2 - a1) * t);

    return 1;
}

/* ↓ color:unpack() ↓ */
static int colorUnpack(lua_State *L) {
    lua_pushnumber(L, getfieldf(L, 1, "r"));
    lua_pushnumber(L, getfieldf(L, 1, "g"));
    lua_pushnumber(L, getfieldf(L, 1, "b"));
    lua_pushnumber(L, getfieldf(L, 1, "a"));
    
    return 4;
}

/* ↓ color :toHSV() metafunction, another alias being :rgbToHSV() ↓ */
static int colorToHSV(lua_State *L) {
    float r = getfieldf(L, 1, "r") / 255.0f;
    float g = getfieldf(L, 1, "g") / 255.0f;
    float b = getfieldf(L, 1, "b") / 255.0f;
    float a = getfieldf(L, 1, "a");

    float min   = fminf(r, fminf(g, b));
    float max   = fmaxf(r, fmaxf(g, b));
    float delta = max - min;

    float h = 0.0f;
    float s = (max > 0.0f) ? (delta / max) : 0.0f;
    float v = max;

    if (delta > 0.0f) {
        if (r == max) {
            h = (g - b) / delta;
        } else if (g == max) {
            h = 2.0f + (b - r) / delta;
        } else {
            h = 4.0f + (r - g) / delta;
        }

        h *= 60.0f;

        if (h < 0.0f) 
            h += 360.0f;
    }

    pushColor(L, h, s * 100.0f, v * 100.0f, a);
    
    return 1;
}

/* ↓ color :toRGB() metafunction, another alias being :hsvToRGB() ↓ */
static int colorToRGB(lua_State *L) {
    double h = getfieldf(L, 1, "r");
    double s = getfieldf(L, 1, "g") / 100.0;
    double v = getfieldf(L, 1, "b") / 100.0;
    float  a = getfieldf(L, 1, "a");
    double r, g, b;
    
    if (s == 0.0) {
        r = g = b = v;
    } else {
        if (h >= 360.0) 
            h = 0.0;

        h /= 60.0;

        int    i = (int)trunc(h);
        double f = h - i;
        double p = v * (1.0 - s);
        double q = v * (1.0 - s * f);
        double t = v * (1.0 - s * (1.0 - f));
        
        switch (i) {
            case 0:  r = v; g = t; b = p; break;
            case 1:  r = q; g = v; b = p; break;
            case 2:  r = p; g = v; b = t; break;
            case 3:  r = p; g = q; b = v; break;
            case 4:  r = t; g = p; b = v; break;
        
            default: r = v; g = p; b = q; break;
        }
    }

    pushColor(L, (float)(r * 255.0), (float)(g * 255.0), (float)(b * 255.0), a);
    
    return 1;
}

static const luaL_Reg colorMethods[] = {
    {"toHSV", colorToHSV},
    {"toRGB", colorToRGB},

    {"hsvToRGB", colorToRGB},
    {"rgbToHSV", colorToHSV},
    
    {"lerp", colorLerp},
    {"unpack", colorUnpack},
    
    {NULL, NULL}
};

static const luaL_Reg colorMeta[] = {
    {"__tostring", toStringColor},
    {"__eq", colorEq},
    
    {NULL, NULL}
};

int Color(lua_State *L) {
    float r = (float)luaL_optnumber(L, 1, 255.0);
    float g = (float)luaL_optnumber(L, 2, 255.0);
    float b = (float)luaL_optnumber(L, 3, 255.0);
    float a = (float)luaL_optnumber(L, 4, 255.0);
    
    pushColor(L, r, g, b, a);
    
    return 1;
}

/* ↓ texture type ↓ */
static const luaL_Reg textureMethods[];
static const luaL_Reg textureMeta[];

void pushTexture(lua_State *L, struct Texture *tex) {
    if (!tex) {
        lua_pushnil(L);

        return;
    }

    lua_newtable(L);
    lua_pushlightuserdata(L, tex);
    lua_setfield(L, -2, "__ptr");

    /* ↓ bake plain read-only fields directly onto the table ↓ */
    lua_pushstring(L, tex->name ? tex->name : "");
    lua_setfield(L, -2, "name");

    lua_pushstring(L, tex->path ? tex->path : "");
    lua_setfield(L, -2, "path");

    lua_pushinteger(L, (lua_Integer)tex->channels);
    lua_setfield(L, -2, "channels");

    lua_pushinteger(L, (lua_Integer)tex->fileSize);
    lua_setfield(L, -2, "fileSize");

    addMethods(L, "vanir.Texture", textureMethods, textureMeta);
}

struct Texture *getTexture(lua_State *L, int idx) {
    struct Texture *tex;

    luaL_checktype(L, idx, LUA_TTABLE);
    lua_getfield(L, idx, "__ptr");
    tex = (struct Texture *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!tex)
        luaL_error(L, "expected texture object");

    return tex;
}

static int toStringTexture(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_pushfstring(L, "Texture: %s (%d x %d)", tex->name ? tex->name : "(unnamed)", (int)tex->width, (int)tex->height);

    return 1;
}

/* ↓ tex:getWidth() → width ↓ */
static int textureGetWidth(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_pushinteger(L, (lua_Integer)tex->width);

    return 1;
}

/* ↓ tex:getHeight() → height ↓ */
static int textureGetHeight(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_pushinteger(L, (lua_Integer)tex->height);

    return 1;
}

/* ↓ tex:getSize() → width, height ↓ */
static int textureGetSize(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    lua_pushinteger(L, (lua_Integer)tex->width);
    lua_pushinteger(L, (lua_Integer)tex->height);

    return 2;
}

/* ↓ tex:getColor(x, y) → Color ↓ */
static int textureMetaGetColor(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);
    uint32_t x = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t y = (uint32_t)luaL_checkinteger(L, 3);

    float r, g, b, a;

    if (!tex->pixels) {
        throw("texture:getColor", tex->name, "no cpu pixel data available");
        lua_pushnil(L);

        return 1;
    }

    if (!textureGetColor(tex, x, y, &r, &g, &b, &a)) {
        throw("texture:getColor", tex->name, "pixel coordinates out of range");
        lua_pushnil(L);

        return 1;
    }

    lua_getglobal(L, "Color");

    if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, r);
        lua_pushnumber(L, g);
        lua_pushnumber(L, b);
        lua_pushnumber(L, a);
        lua_call(L, 4, 1);

        return 1;
    }

    lua_pop(L, 1);

    lua_newtable(L);
    setFieldNumber(L, "r", r);
    setFieldNumber(L, "g", g);
    setFieldNumber(L, "b", b);
    setFieldNumber(L, "a", a);

    return 1;
}

/* ↓ tex:draw(sx, sy, sw, sh, dx, dy [, dw, dh]) ↓ */
/* ↓ sets this texture as active then calls drawTexturedRect ↓ */
static int textureDraw(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    /* ↓ temporarily set as active so drawTexturedRect can find it ↓ */
    extern struct Texture *activeTexture;
    activeTexture = tex;

    /* ↓ shift args down: remove self from stack so drawTexturedRect sees sx at 1 ↓ */
    lua_remove(L, 1);

    return drawTexturedRect(L);
}

/* ↓ tex:release() — frees GPU resources and invalidates this object ↓ */
static int textureReleaseMeta(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    textureRelease(tex);

    lua_pushnil(L);
    lua_setfield(L, 1, "__ptr");

    return 0;
}

/* ↓ tex:setImage(path) — loads a new image into this texture from a file ↓ */
/* ↓ path defaults to tex.path so tex:setImage() reloads the original file ↓ */
static int textureSetImageMeta(lua_State *L) {
    struct Texture *tex = getTexture(L, 1);

    /* ↓ default to the texture's current path so :setImage() acts as a reload ↓ */
    const char *path = luaL_optstring(L, 2, tex->path);

    if (!path) {
        throw("texture:setImage", tex->name, "no path — provide a path or load from a file first");
        lua_pushboolean(L, 0);

        return 1;
    }

    if (!textureSetImage(tex, path)) {
        lua_pushboolean(L, 0);

        return 1;
    }

    /* ↓ update the baked fields to reflect new image ↓ */
    lua_pushstring(L, tex->path);
    lua_setfield(L, 1, "path");

    lua_pushinteger(L, (lua_Integer)tex->channels);
    lua_setfield(L, 1, "channels");

    lua_pushinteger(L, (lua_Integer)tex->fileSize);
    lua_setfield(L, 1, "fileSize");

    lua_pushboolean(L, 1);

    return 1;
}

static const luaL_Reg textureMethods[] = {
    {"getWidth",  textureGetWidth},
    {"getHeight", textureGetHeight},
    {"getSize",   textureGetSize},
    {"getColor",  textureMetaGetColor},
    {"draw",      textureDraw},
    {"release",   textureReleaseMeta},
    {"setImage",  textureSetImageMeta},

    {NULL, NULL}
};

static const luaL_Reg textureMeta[] = {
    {"__tostring", toStringTexture},

    {NULL, NULL}
};

/* ↓ quaternion type ↓ */
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
    {"length",     quatLength},
    {"normalize",  quatNormalize},
    {"conjugate",  quatConjugate},
    {"dot",        quatDot},
    {"slerp",      quatSlerp},
    {"toAngle",    quatToAngle},

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

/* ↓ file type ↓ */
/* ↓ pull the File ptr from the lua object; errors if dead ↓ */
struct File *getFile(lua_State *L, int idx) {
    luaL_checktype(L, idx, LUA_TTABLE);
    lua_getfield(L, idx, "__ptr");

    struct File *f = (struct File *)lua_touserdata(L, -1);

    lua_pop(L, 1);

    if (!f || !f->handle)
        luaL_error(L, "file is closed");

    return f;
}

/* ↓ push a new File lua object from an already-open FILE* ↓ */
void pushFile(lua_State *L, FILE *handle, const char *path) {
    extern const luaL_Reg fileMethods[];
    extern const luaL_Reg fileMeta[];

    struct File *f = (struct File *)malloc(sizeof(struct File));

    if (!f) {
        throw("File", path, "Memory allocation error");
        lua_pushnil(L);

        return;
    }

    f->handle = handle;
    f->path = path ? strdup(path) : NULL;

    lua_newtable(L);
    lua_pushlightuserdata(L, f);
    lua_setfield(L, -2, "__ptr");
    lua_pushstring(L, path ? path : "");
    lua_setfield(L, -2, "path");

    /* ↓ add methods and metatable ↓ */
    luaL_newmetatable(L, "vanir.File");

    lua_newtable(L);

    for (const luaL_Reg *m = fileMethods; m->name; ++m) {
        lua_pushcfunction(L, m->func);
        lua_setfield(L, -2, m->name);
    }

    lua_setfield(L, -2, "__index");

    for (const luaL_Reg *m = fileMeta; m->name; ++m) {
        lua_pushcfunction(L, m->func);
        lua_setfield(L, -2, m->name);
    }

    lua_setmetatable(L, -2);
}
