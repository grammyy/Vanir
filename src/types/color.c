#include "common.h"

#include <math.h>

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

/* ↓ color:toHSV() metafunction, another alias being :rgbToHSV() ↓ */
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

/* ↓ color:toRGB() metafunction, another alias being :hsvToRGB() ↓ */
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
    {"toHSV",    colorToHSV},
    {"toRGB",    colorToRGB},

    {"hsvToRGB", colorToRGB},
    {"rgbToHSV", colorToHSV},

    {"lerp",     colorLerp},
    {"unpack",   colorUnpack},

    {NULL, NULL}
};

static const luaL_Reg colorMeta[] = {
    {"__tostring", toStringColor},
    {"__eq",       colorEq},

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