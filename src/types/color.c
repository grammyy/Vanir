#include "common.h"

#include <math.h>

static const luaL_Reg colorMethods[];
static const luaL_Reg colorMeta[];

struct VanirCol *pushColor(lua_State *L, float r, float g, float b, float a) {
    struct VanirCol *c = (struct VanirCol *)lua_newuserdata(L, sizeof(struct VanirCol));
    
    c->r = r;
    c->g = g;
    c->b = b;
    c->a = a;

    addMethodsUD(L, "vanir.Color", colorMethods, colorMeta);

    return c;
}

static int toStringColor(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);

    lua_pushfstring(L, "(%f, %f, %f, %f)", c->r, c->g, c->b, c->a);

    return 1;
}

/* ↓ __concat: Color .. Color  or  Color .. string ↓ */
static int colorConcat(lua_State *L) {
    lua_getglobal(L, "tostring");
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);
    /* ↑ convert both operands to string, concatenate ↑ */

    const char *s1 = lua_tostring(L, -1);

    lua_pop(L, 1);

    lua_getglobal(L, "tostring");
    lua_pushvalue(L, 2);
    lua_call(L, 1, 1);

    const char *s2 = lua_tostring(L, -1);

    lua_pop(L, 1);
    lua_pushfstring(L, "%s%s", s1 ? s1 : "", s2 ? s2 : "");

    return 1;
}

/* ↓ __eq ↓ */
static int colorEq(lua_State *L) {
    struct VanirCol *a = checkCol(L, 1);
    struct VanirCol *b = checkCol(L, 2);

    lua_pushboolean(L, a->r == b->r && a->g == b->g && a->b == b->b && a->a == b->a);

    return 1;
}

/* ↓ __add: component-wise ↓ */
static int colorAdd(lua_State *L) {
    struct VanirCol *a = checkCol(L, 1);
    struct VanirCol *b = checkCol(L, 2);

    pushColor(L, a->r + b->r, a->g + b->g, a->b + b->b, a->a + b->a);

    return 1;
}

/* ↓ __sub: component-wise ↓ */
static int colorSub(lua_State *L) {
    struct VanirCol *a = checkCol(L, 1);
    struct VanirCol *b = checkCol(L, 2);

    pushColor(L, a->r - b->r, a->g - b->g, a->b - b->b, a->a - b->a);

    return 1;
}

/* ↓ __mul: Color * scalar  or  scalar * Color ↓ */
static int colorMul(lua_State *L) {
    float r, g, b, a, s;

    if (lua_isnumber(L, 1)) {
        s = (float)lua_tonumber(L, 1);
        struct VanirCol *c = checkCol(L, 2);
        r = c->r; g = c->g; b = c->b; a = c->a;
    } else {
        struct VanirCol *c = checkCol(L, 1);
        r = c->r; g = c->g; b = c->b; a = c->a;
        s = (float)luaL_checknumber(L, 2);
    }

    pushColor(L, r * s, g * s, b * s, a * s);

    return 1;
}

/* ↓ __div: Color / scalar ↓ */
static int colorDiv(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);
    float s = (float)luaL_checknumber(L, 2);

    pushColor(L, c->r / s, c->g / s, c->b / s, c->a / s);

    return 1;
}

/* ↓ :toHex() → string "#RRGGBB" or "#RRGGBBAA" ↓ */
static int colorToHex(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);
    int r = (int)c->r, g = (int)c->g, b = (int)c->b, a = (int)c->a;
    char buf[12];

    if (a == 255) {
        snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
    } else {
        snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", r, g, b, a);
    }

    lua_pushstring(L, buf);

    return 1;
}

/* ↓ :round([decimals]) → new Color ↓ */
static int colorRound(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);
    float mul = 1.0f;

    if (!lua_isnoneornil(L, 2)) {
        int dec = (int)lua_tointeger(L, 2);

        for (int i = 0; i < dec; i++)
            mul *= 10.0f;
    }

    pushColor(L,
        roundf(c->r * mul) / mul,
        roundf(c->g * mul) / mul,
        roundf(c->b * mul) / mul,
        roundf(c->a * mul) / mul
    );

    return 1;
}

/* ↓ :clone() ↓ */
static int colorClone(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);

    pushColor(L, c->r, c->g, c->b, c->a);

    return 1;
}

/* ↓ :set(r, g, b [, a]) — mutates in-place, returns self ↓ */
static int colorSet(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);

    c->r = (float)luaL_checknumber(L, 2);
    c->g = (float)luaL_checknumber(L, 3);
    c->b = (float)luaL_checknumber(L, 4);

    if (!lua_isnoneornil(L, 5))
        c->a = (float)lua_tonumber(L, 5);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setR(v) ↓ */
static int colorSetR(lua_State *L) {
    checkCol(L, 1)->r = (float)luaL_checknumber(L, 2);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setG(v) ↓ */
static int colorSetG(lua_State *L) {
    checkCol(L, 1)->g = (float)luaL_checknumber(L, 2);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setB(v) ↓ */
static int colorSetB(lua_State *L) {
    checkCol(L, 1)->b = (float)luaL_checknumber(L, 2);

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setA(v) ↓ */
static int colorSetA(lua_State *L) {
    checkCol(L, 1)->a = (float)luaL_checknumber(L, 2);
    
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ color:lerp(other, t) linear interpolate between two colors ↓ */
static int colorLerp(lua_State *L) {
    struct VanirCol *a = checkCol(L, 1);
    struct VanirCol *b = checkCol(L, 2);
    float t = (float)luaL_checknumber(L, 3);

    pushColor(L,
        a->r + (b->r - a->r) * t,
        a->g + (b->g - a->g) * t,
        a->b + (b->b - a->b) * t,
        a->a + (b->a - a->a) * t
    );

    return 1;
}

/* ↓ :unpack() → r, g, b, a ↓ */
static int colorUnpack(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);

    lua_pushnumber(L, c->r);
    lua_pushnumber(L, c->g);
    lua_pushnumber(L, c->b);
    lua_pushnumber(L, c->a);

    return 4;
}

/* ↓ color:toHSV() metafunction, another alias being :rgbToHSV() ↓ */
static int colorToHSV(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);
    float r = c->r / 255.0f;
    float g = c->g / 255.0f;
    float b = c->b / 255.0f;
    float a = c->a;

    float mn   = fminf(r, fminf(g, b));
    float mx   = fmaxf(r, fmaxf(g, b));
    float delta = mx - mn;

    float h = 0.0f;
    float s = (mx > 0.0f) ? (delta / mx) : 0.0f;
    float v = mx;

    if (delta > 0.0f) {
        if (r == mx) {
            h = (g - b) / delta;
        } else if (g == mx) {
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

/* ↓ color:toRGB() metafunction, another alias being :hsvToRGB() ↓ */
static int colorToRGB(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);
    double h = c->r;
    double s = c->g / 100.0;
    double v = c->b / 100.0;
    float  a = c->a;
    double r, g, b;

    if (s == 0.0) {
        r = g = b = v;
    } else {
        if (h >= 360.0)
            h = 0.0;

        h /= 60.0;

        int i = (int)trunc(h);
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

/* ↓ __index — expose r/g/b/a fields from userdata to Lua ↓ */
static int colorIndex(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);
    const char *key = luaL_checkstring(L, 2);

    if (key[1] == '\0') {
        if (key[0] == 'r') { lua_pushnumber(L, c->r); return 1; }
        if (key[0] == 'g') { lua_pushnumber(L, c->g); return 1; }
        if (key[0] == 'b') { lua_pushnumber(L, c->b); return 1; }
        if (key[0] == 'a') { lua_pushnumber(L, c->a); return 1; }
    }

    /* ↓ fall through to method table ↓ */
    vanirUD_indexFallback(L, "vanir.Color", key);

    return 1;
}

/* ↓ __newindex — allow c.r = n style assignment ↓ */
static int colorNewIndex(lua_State *L) {
    struct VanirCol *c = checkCol(L, 1);
    const char *key = luaL_checkstring(L, 2);
    float val = (float)luaL_checknumber(L, 3);

    if (key[1] == '\0') {
        if (key[0] == 'r') { c->r = val; return 0; }
        if (key[0] == 'g') { c->g = val; return 0; }
        if (key[0] == 'b') { c->b = val; return 0; }
        if (key[0] == 'a') { c->a = val; return 0; }
    }

    return luaL_error(L, "Color has no field '%s'", key);
}

static const luaL_Reg colorMethods[] = {
    {"rgbToHSV", colorToHSV},
    {"hsvToRGB", colorToRGB},
    {"toHSV",    colorToHSV},
    {"toRGB",    colorToRGB},
    {"toHex",    colorToHex},
    {"round",    colorRound},
    {"clone",    colorClone},
    {"set",      colorSet},
    {"setR",     colorSetR},
    {"setG",     colorSetG},
    {"setB",     colorSetB},
    {"setA",     colorSetA},
    {"lerp",     colorLerp},
    {"unpack",   colorUnpack},

    {NULL, NULL}
};

static const luaL_Reg colorMeta[] = {
    {"__tostring", toStringColor},
    {"__concat",   colorConcat},
    {"__eq",       colorEq},
    {"__add",      colorAdd},
    {"__sub",      colorSub},
    {"__mul",      colorMul},
    {"__div",      colorDiv},
    {"__index",    colorIndex},
    {"__newindex", colorNewIndex},

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