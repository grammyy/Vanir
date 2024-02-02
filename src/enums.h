#ifndef VANIR_ENUMS
#define VANIR_ENUMS

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct {
    const char* name;
    int value;
} enums;

int renderEnums(lua_State* L) {
    enums render[] = {
        {"1-alpha", 14},
        {"alpha", 13},
        {"1-color", 12},
        {"color", 11},
        {"1-dstAlpha", 10},
        {"distAlpha", 9},
        {"1-srcAlpha", 8},
        {"srcAlpha", 7},
        {"1-DstColor", 6},
        {"dstColor", 5},
        {"1-srcColor", 4},
        {"srcColor", 3},
        {"one", 2},
        {"zero", 1},

        {"blend", 28},
        {"clipDist", 27},
        {"colorLogic", 26},
        {"cullFace", 25},
        {"debug", 24},
        {"debugSync", 23},
        {"depthClamp", 22},
        {"depthTest", 21},
        {"dither", 20},
        {"framebuffer", 19},
        {"lineSmooth", 18},
        {"multisample", 17},
        {"polygonFill", 16},
        {"polygonLine", 15},
        {"polygonPoint", 14},
        {"polygonSmooth", 13},
        {"primitive", 12},
        {"primitiveIndex", 11},
        {"rasterizerDisgard", 10},
        {"alphaCoverage", 9},
        {"AlphaToOne", 8},
        {"sampleCoverage", 7},
        {"sampleShading", 6},
        {"sampleMask", 5},
        {"scissor", 4},
        {"stencil", 3},
        {"texture", 2},
        {"pointSize", 1},
        
        {"shaders", 4},
        {"lines", 3},
        {"polygons", 2},
        {"texture", 1},

        {"fastest", 3},
        {"nicest", 2},
        {"system", 1},
        {NULL, 0}
    };

    lua_newtable(L);

    for (int i = 0; render[i].name != NULL; ++i) {
        lua_pushstring(L, render[i].name);
        lua_pushinteger(L, render[i].value);
        lua_settable(L, -3);
    }

    return 1;
}

#endif