/* types.c */
#include "types.h"
#include "vanir.h"
#include "graphics/textures.h"

#include <math.h>


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
