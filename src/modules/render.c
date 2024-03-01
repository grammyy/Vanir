#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "windows.h"
#include "render.h"
#include "../graphics/render.h"
#include "../vanir.h"

#define blendEnums 15
#define capEnums 28
#define hintEnums 4
#define qualityEnums 28
#define groupEnums 10
#define bufferEnums 3

const GLenum blendLookup[blendEnums] = {
    GL_ZERO,
    GL_ONE,
    GL_SRC_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_DST_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_CONSTANT_COLOR,
    GL_ONE_MINUS_CONSTANT_COLOR,
    GL_CONSTANT_ALPHA,
    GL_ONE_MINUS_CONSTANT_ALPHA,
    GL_SRC_ALPHA_SATURATE
};

const GLenum capLookup[capEnums] = {
    GL_PROGRAM_POINT_SIZE,
    GL_TEXTURE_CUBE_MAP_SEAMLESS,
    GL_STENCIL_TEST,
    GL_SCISSOR_TEST,
    GL_SAMPLE_MASK,
    GL_SAMPLE_SHADING,
    GL_SAMPLE_COVERAGE,
    GL_SAMPLE_ALPHA_TO_ONE,
    GL_SAMPLE_ALPHA_TO_COVERAGE,
    GL_RASTERIZER_DISCARD,
    GL_PRIMITIVE_RESTART_FIXED_INDEX,
    GL_PRIMITIVE_RESTART,
    GL_POLYGON_SMOOTH,
    GL_POLYGON_OFFSET_POINT,
    GL_POLYGON_OFFSET_LINE,
    GL_POLYGON_OFFSET_FILL,
    GL_MULTISAMPLE,
    GL_LINE_SMOOTH,
    GL_FRAMEBUFFER_SRGB,
    GL_DITHER,
    GL_DEPTH_TEST,
    GL_DEPTH_CLAMP,
    GL_DEBUG_OUTPUT_SYNCHRONOUS,
    GL_DEBUG_OUTPUT,
    GL_CULL_FACE,
    GL_COLOR_LOGIC_OP,
    // Placeholder for 27: GL_CLIP_DISTANCE,
    GL_BLEND,
};

const GLenum hintLookup[hintEnums] = {
    GL_TEXTURE_COMPRESSION_HINT,
    GL_POLYGON_SMOOTH_HINT,
    GL_LINE_SMOOTH_HINT,
    GL_FRAGMENT_SHADER_DERIVATIVE_HINT,
};

const GLenum qualityLookup[qualityEnums] = {
    GL_DONT_CARE,
    GL_NICEST,
    GL_FASTEST,
};

const GLenum groupLookup[groupEnums] = {
    GL_POLYGON,
    GL_QUAD_STRIP,
    GL_QUADS,
    GL_TRIANGLE_FAN,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLES,
    GL_LINE_LOOP,
    GL_LINE_STRIP,
    GL_LINES,
    GL_POINTS,
};

const GLenum bufferLookup[bufferEnums] = {
    GL_COLOR_BUFFER_BIT,
    GL_DEPTH_BUFFER_BIT,
    GL_STENCIL_BUFFER_BIT,
};

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

    glHint(hintLookup[target], qualityLookup[quality]);

    return 0;
}

int setBlend(lua_State *L) {
    int source = luaL_checkinteger(L, 1);
    int blend = luaL_checkinteger(L, 2);

    glBlendFunc(blendLookup[source], blendLookup[blend]);

    return 0;
}

int enable(lua_State *L) {
    int cap = luaL_checkinteger(L, 1);

    glEnable(capLookup[cap]);

    return 0;
}

int disable(lua_State *L) {
    int cap = luaL_checkinteger(L, 1);

    glDisable(capLookup[cap]);

    return 0;
}

int clear(lua_State *L) {
    struct color color;
    getColor(L, &color);

    int buffer = lua_tointeger(L, 2);
    
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(bufferLookup[buffer]);
    
    return 0;
}

int force(lua_State *L) {
    glFlush();
    
    return 0;
}

int begin(lua_State *L) {
    int group = luaL_checkinteger(L, 1);

    glBegin(groupLookup[group]);

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

    {"selectTexture", selectTexture},
    {"newTexture", newTexture},
    {"destroyTexture", destroyTexture},
    {"drawTexture", drawTexture},

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