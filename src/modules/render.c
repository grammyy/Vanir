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
    switch (blend) { //convert these to structs later
        case 15: return GL_SRC_ALPHA_SATURATE;
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
    switch (cap) { //convert these to structs later
        case 28: return GL_BLEND;
        //case 27: return GL_CLIP_DISTANCE; undefined for some reason
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

int destroy(lua_State *L) {
    struct sdlWindow **window = (struct sdlWindow **)luaL_checkudata(L, 1, "window");
    
    SDL_GL_DeleteContext((*window)->context);

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

int setColor(lua_State *L) {
    struct color color;
    getColor(L, &color);

    lua_getglobal(L, "_rendercolor");

    lua_pushnumber(L, color.r * 255.0f);
    lua_setfield(L, -2, "r");

    lua_pushnumber(L, color.g * 255.0f);
    lua_setfield(L, -2, "g");

    lua_pushnumber(L, color.b * 255.0f);
    lua_setfield(L, -2, "b");

    lua_pushnumber(L, color.a * 255.0f);
    lua_setfield(L, -2, "a");

    return 0;
}

int setQuality(lua_State *L) {
    int target = luaL_checkinteger(L, 1);
    int quality = luaL_checkinteger(L, 2);

    switch (target) { //convert these to structs later
        case 4: target = GL_FRAGMENT_SHADER_DERIVATIVE_HINT; break;
        case 3: target = GL_LINE_SMOOTH_HINT; break;
        case 2: target = GL_POLYGON_SMOOTH_HINT; break;
        case 1: target = GL_TEXTURE_COMPRESSION_HINT; break;
    }

    switch (quality) {
        case 3: quality = GL_FASTEST; break;
        case 2: quality = GL_NICEST; break;
        case 1: quality = GL_DONT_CARE; break;
    }

    glHint(target, quality);

    return 0;
}

int setBlend(lua_State *L) {
    int source = luaL_checkinteger(L, 1);
    int blend = luaL_checkinteger(L, 2);

    source=blendSwitch(source);
    blend=blendSwitch(blend);
    
    glBlendFunc(source, blend);

    return 0;
}

int enable(lua_State *L) {
    int cap = luaL_checkinteger(L, 1);
    cap=capSwitch(cap);

    glEnable(cap);

    return 0;
}

int disable(lua_State *L) {
    int cap = luaL_checkinteger(L, 1);
    cap=capSwitch(cap);

    glDisable(cap);

    return 0;
}

int clear(lua_State *L) {
    struct color color;
    getColor(L, &color);

    int buffer = lua_tointeger(L, 2);
    
    switch (buffer) { //convert these to structs later
        case 3: buffer = GL_STENCIL_BUFFER_BIT; break;
        case 2: buffer = GL_DEPTH_BUFFER_BIT; break;
        case 1: default: buffer = GL_COLOR_BUFFER_BIT; break;
    }

    glClearColor(color.r, color.g, color.b, color.a);
    glClear(buffer);
    
    return 0;
}

int force(lua_State *L) {
    glFlush();
    
    return 0;
}

int begin(lua_State *L) {
    int group = luaL_checkinteger(L, 1);

    switch (group) { //convert these to structs later
        case 10: group = GL_POINTS; break;
        case 9: group = GL_LINES; break;
        case 8: group = GL_LINE_STRIP; break;
        case 7: group = GL_LINE_LOOP; break;
        case 6: group = GL_TRIANGLES; break;
        case 5: group = GL_TRIANGLE_STRIP; break;
        case 4: group = GL_TRIANGLE_FAN; break;
        case 3: group = GL_QUADS; break;
        case 2: group = GL_QUAD_STRIP; break;
        case 1: group = GL_POLYGON; break;
    }

    glBegin(group);

    return 0;
}

int end(lua_State *L) {
    glEnd();

    return 0;
}

int scissor(lua_State *L) {
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);
    float width = lua_tonumber(L, 3);
    float height = lua_tonumber(L, 4);

    glScissor(x, y, width, height);

    return 0;
}

int resetMatrix(lua_State *L) {
    glLoadIdentity();

    return 0;
}

int pushMatrix(lua_State *L) {
    glPushMatrix();

    return 0;
}

int popMatrix(lua_State *L) {
    glPopMatrix();

    return 0;
}

const luaL_Reg luaRender[] = {
    {"drawLine", drawLine},
    {"drawRect", drawRect},
    {"drawCircle", drawCircle},
    {"drawFilledCircle", drawFilledCircle},
    {"drawPoly", drawPoly},
    {"drawVertex", drawVertex},

    {"clear", clear},
    {"setQuality", setQuality},
    {"setBlend", setBlend},
    {"enable", enable},
    {"disable", disable},
    {"setColor", setColor},
    {"force", force},
    {"begin", begin},
    {"exit", end},
    {"scissor", scissor},
    {"resetMatrix", resetMatrix},
    {"pushMatrix", pushMatrix},
    {"popMatrix", popMatrix},
    {NULL, NULL}
};

int renderInit(lua_State* L) {
    luaL_newlib(L, luaRender);

    return 1;
}