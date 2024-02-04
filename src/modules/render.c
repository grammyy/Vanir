#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "windows.h"
#include "render.h"
#include "../graphics/render.h"
#include "../vanir.h"

GLenum blendSwitch(int blend) {
    switch (blend) {
        case 14: return GL_ONE_MINUS_CONSTANT_ALPHA;
        case 13: return GL_CONSTANT_ALPHA;
        case 12: return GL_ONE_MINUS_CONSTANT_COLOR;
        case 11: return GL_CONSTANT_COLOR;
        case 10: return GL_ONE_MINUS_DST_ALPHA;
        case 9:  return GL_DST_ALPHA;
        case 8:  return GL_ONE_MINUS_SRC_ALPHA;
        case 7:  return GL_SRC_ALPHA;
        case 6:  return GL_ONE_MINUS_DST_COLOR;
        case 5:  return GL_DST_COLOR;
        case 4:  return GL_ONE_MINUS_SRC_COLOR;
        case 3:  return GL_SRC_COLOR;
        case 2:  return GL_ONE;
        case 1:  return GL_ZERO;
        default: return GL_INVALID_ENUM;
    }
}

GLenum capSwitch(int cap) {
    switch (cap) {
        case 28: return GL_BLEND;
        //case 27: return GL_CLIP_DISTANCE;
        case 26: return GL_COLOR_LOGIC_OP;
        case 25: return GL_CULL_FACE;
        case 24: return GL_DEBUG_OUTPUT;
        case 23: return GL_DEBUG_OUTPUT_SYNCHRONOUS;
        case 22: return GL_DEPTH_CLAMP;
        case 21: return GL_DEPTH_TEST;
        case 20: return GL_DITHER;
        case 19: return GL_FRAMEBUFFER_SRGB;
        case 18: return GL_LINE_SMOOTH;
        case 17: return GL_MULTISAMPLE;
        case 16: return GL_POLYGON_OFFSET_FILL;
        case 15: return GL_POLYGON_OFFSET_LINE;
        case 14: return GL_POLYGON_OFFSET_POINT;
        case 13: return GL_POLYGON_SMOOTH;
        case 12: return GL_PRIMITIVE_RESTART;
        case 11: return GL_PRIMITIVE_RESTART_FIXED_INDEX;
        case 10: return GL_RASTERIZER_DISCARD;
        case 9:  return GL_SAMPLE_ALPHA_TO_COVERAGE;
        case 8:  return GL_SAMPLE_ALPHA_TO_ONE;
        case 7:  return GL_SAMPLE_COVERAGE;
        case 6:  return GL_SAMPLE_SHADING;
        case 5:  return GL_SAMPLE_MASK;
        case 4:  return GL_SCISSOR_TEST;
        case 3:  return GL_STENCIL_TEST;
        case 2:  return GL_TEXTURE_CUBE_MAP_SEAMLESS;
        case 1:  return GL_PROGRAM_POINT_SIZE;
        default: return GL_INVALID_ENUM;
    }
}

// window methods ↓↓↓ window methods ///
int selectRender(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");
    
    SDL_GL_MakeCurrent((*window)->window, (*window)->context);

    return 0;
}

int stopRender(lua_State *L) {
    SDL_GL_MakeCurrent(NULL, NULL);

    return 0;
}

int update(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");
    
    SDL_GL_SwapWindow((*window)->window);

    return 0;
}
// window methods ↑↑↑ window methods ///

void getGlobalColor(lua_State *L, struct color *color) {
    lua_getglobal(L, "_rendercolor");

    lua_getfield(L, -1, "r");
    color->r = lua_tointeger(L, -1) / 255.0f;
    lua_pop(L, 1);

    lua_getfield(L, -1, "g");
    color->g = lua_tointeger(L, -1) / 255.0f;
    lua_pop(L, 1);

    lua_getfield(L, -1, "b");
    color->b = lua_tointeger(L, -1) / 255.0f;
    lua_pop(L, 1);

    lua_getfield(L, -1, "a");
    color->a = lua_tointeger(L, -1) / 255.0f;
    lua_pop(L, 2);
}

void getColor(lua_State *L, struct color *color) {
    lua_getfield(L, 1, "r");
    color->r = lua_tointeger(L, -1) / 255.0f;
    lua_pop(L, 1);

    lua_getfield(L, 1, "g");
    color->g = lua_tointeger(L, -1) / 255.0f;
    lua_pop(L, 1);

    lua_getfield(L, 1, "b");
    color->b = lua_tointeger(L, -1) / 255.0f;
    lua_pop(L, 1);

    lua_getfield(L, 1, "a");
    color->a = lua_tointeger(L, -1) / 255.0f;
    lua_pop(L, 1);
}

int setQuality(lua_State *L) {
    int target = lua_tonumber(L, 1);
    int quality = lua_tonumber(L, 2);

    switch (target) {
        case 4:
            target=GL_FRAGMENT_SHADER_DERIVATIVE_HINT;
            break;
        case 3:
            target=GL_LINE_SMOOTH_HINT;
            break;
        case 2:
            target=GL_POLYGON_SMOOTH_HINT;
            break;
        case 1:
            target=GL_TEXTURE_COMPRESSION_HINT;
            break;
    }

    switch (quality) {
        case 3:
            quality=GL_FASTEST;
            break;
        case 2:
            quality=GL_NICEST;
            break;
        case 1:
            quality=GL_DONT_CARE;
            break;
    }

    glHint(target, quality);

    return 0;
}

int setBlend(lua_State *L) {
    int source = lua_tonumber(L, 1);
    int blend = lua_tonumber(L, 2);

    source=blendSwitch(source);
    blend=blendSwitch(blend);
    
    glBlendFunc(source, blend);

    return 0;
}

int enable(lua_State *L) {
    int cap = lua_tonumber(L, 1);

    cap=capSwitch(cap);

    glEnable(cap);

    return 0;
}

int disable(lua_State *L) {
    int cap = lua_tonumber(L, 1);

    cap=capSwitch(cap);

    glDisable(cap);

    return 0;
}

int clear(lua_State *L) {
    struct color color;

    getColor(L, &color);

    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
    
    return 0;
}

const luaL_Reg luaRender[] = {
    {"drawLine", drawLine},
    {"clear", clear},
    {"setQuality", setQuality},
    {"setBlend", setBlend},
    {"enable", enable},
    {"disable", disable},
    {NULL, NULL}
};

int renderInit(lua_State* L) {
    luaL_newlib(L, luaRender);

    return 1;
}