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

/* ↓ __index: numeric 1/2/3/4 → r/g/b/a, then method table ↓ */
/* ↓ do i really need this ↓ */
static int colorIndex(lua_State *L) {
    if (lua_isnumber(L, 2)) {
        int k = (int)lua_tointeger(L, 2);
        const char *field = (k == 1) ? "r" : (k == 2) ? "g" : (k == 3) ? "b" : (k == 4) ? "a" : NULL;
        
        if (field) { 
            lua_getfield(L, 1, field); 
            
            return 1; 
        }
    }

    luaL_getmetatable(L, "vanir.Color");
    lua_getfield(L, -1, "__index");

    if (lua_istable(L, -1)) {
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);

        return 1;
    }

    lua_pushnil(L);

    return 1;
}

/* ↓ __concat: Color .. Color  or  Color .. string ↓ */
static int colorConcat(lua_State *L) {
    /* ↓ convert both operands to string, concatenate ↓ */
    lua_getglobal(L, "tostring");
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);

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
    lua_pushboolean(L,
        getfieldf(L, 1, "r") == getfieldf(L, 2, "r") &&
        getfieldf(L, 1, "g") == getfieldf(L, 2, "g") &&
        getfieldf(L, 1, "b") == getfieldf(L, 2, "b") &&
        getfieldf(L, 1, "a") == getfieldf(L, 2, "a")
    );

    return 1;
}

/* ↓ __add: component-wise ↓ */
static int colorAdd(lua_State *L) {
    pushColor(L,
        getfieldf(L, 1, "r") + getfieldf(L, 2, "r"),
        getfieldf(L, 1, "g") + getfieldf(L, 2, "g"),
        getfieldf(L, 1, "b") + getfieldf(L, 2, "b"),
        getfieldf(L, 1, "a") + getfieldf(L, 2, "a")
    );

    return 1;
}

/* ↓ __sub: component-wise ↓ */
static int colorSub(lua_State *L) {
    pushColor(L,
        getfieldf(L, 1, "r") - getfieldf(L, 2, "r"),
        getfieldf(L, 1, "g") - getfieldf(L, 2, "g"),
        getfieldf(L, 1, "b") - getfieldf(L, 2, "b"),
        getfieldf(L, 1, "a") - getfieldf(L, 2, "a")
    );

    return 1;
}

/* ↓ __mul: Color * scalar  or  scalar * Color ↓ */
static int colorMul(lua_State *L) {
    float r, g, b, a, s;

    if (lua_isnumber(L, 1)) {
        s = (float)lua_tonumber(L, 1);
        r = getfieldf(L, 2, "r"); g = getfieldf(L, 2, "g");
        b = getfieldf(L, 2, "b"); a = getfieldf(L, 2, "a");
    } else {
        r = getfieldf(L, 1, "r"); g = getfieldf(L, 1, "g");
        b = getfieldf(L, 1, "b"); a = getfieldf(L, 1, "a");
        s = (float)luaL_checknumber(L, 2);
    }

    pushColor(L, r * s, g * s, b * s, a * s);

    return 1;
}

/* ↓ __div: Color / scalar ↓ */
static int colorDiv(lua_State *L) {
    float r = getfieldf(L, 1, "r"), g = getfieldf(L, 1, "g");
    float b = getfieldf(L, 1, "b"), a = getfieldf(L, 1, "a");
    float s = (float)luaL_checknumber(L, 2);

    pushColor(L, r / s, g / s, b / s, a / s);

    return 1;
}

/* ↓ :toHex() → string "#RRGGBB" or "#RRGGBBAA" ↓ */
static int colorToHex(lua_State *L) {
    int r = (int)getfieldf(L, 1, "r");
    int g = (int)getfieldf(L, 1, "g");
    int b = (int)getfieldf(L, 1, "b");
    int a = (int)getfieldf(L, 1, "a");

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
    float r = getfieldf(L, 1, "r"), g = getfieldf(L, 1, "g");
    float b = getfieldf(L, 1, "b"), a = getfieldf(L, 1, "a");
    float mul = 1.0f;

    if (!lua_isnoneornil(L, 2)) {
        int dec = (int)lua_tointeger(L, 2);

        for (int i = 0; i < dec; i++) 
            mul *= 10.0f;
    }

    pushColor(L,
        roundf(r * mul) / mul,
        roundf(g * mul) / mul,
        roundf(b * mul) / mul,
        roundf(a * mul) / mul
    );

    return 1;
}

/* ↓ :clone() ↓ */
static int colorClone(lua_State *L) {
    pushColor(L,
        getfieldf(L, 1, "r"),
        getfieldf(L, 1, "g"),
        getfieldf(L, 1, "b"),
        getfieldf(L, 1, "a")
    );

    return 1;
}

/* ↓ :set(r, g, b [, a]) — mutates in-place, returns self ↓ */
static int colorSet(lua_State *L) {
    setFieldNumber(L, "r", (float)luaL_checknumber(L, 2));
    setFieldNumber(L, "g", (float)luaL_checknumber(L, 3));
    setFieldNumber(L, "b", (float)luaL_checknumber(L, 4));

    if (!lua_isnoneornil(L, 5))
        setFieldNumber(L, "a", (float)lua_tonumber(L, 5));

    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setR(v) ↓ */
static int colorSetR(lua_State *L) {
    setFieldNumber(L, "r", (float)luaL_checknumber(L, 2));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setG(v) ↓ */
static int colorSetG(lua_State *L) {
    setFieldNumber(L, "g", (float)luaL_checknumber(L, 2));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setB(v) ↓ */
static int colorSetB(lua_State *L) {
    setFieldNumber(L, "b", (float)luaL_checknumber(L, 2));
    lua_pushvalue(L, 1);

    return 1;
}

/* ↓ :setA(v) ↓ */
static int colorSetA(lua_State *L) {
    setFieldNumber(L, "a", (float)luaL_checknumber(L, 2));
    lua_pushvalue(L, 1);

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
        a1 + (a2 - a1) * t
    );

    return 1;
}

/* ↓ :unpack() → r, g, b, a ↓ */
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
    {"__index",    colorIndex},
    {"__concat",   colorConcat},
    {"__eq",       colorEq},
    {"__add",      colorAdd},
    {"__sub",      colorSub},
    {"__mul",      colorMul},
    {"__div",      colorDiv},

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