#include <SDL.h>
#include <SDL_opengl.h>
#include "render.h"
#include <math.h>
#include "../modules/render.h"
#include "../vanir.h"

void circle(float x, float y, float radius, int segments, lua_State *L) {
    float theta = (2.0f * M_PI) / (float)segments;
    float c = cosf(theta);
    float s = sinf(theta);
    float cx = radius;
    float cy = 0;
    struct color color;

    if (!lua_isfunction(L, 5)) {
        getGlobalColor(L, &color);
        glColor4f(color.r, color.g, color.b, color.a);
    }

    for (int i = 0; i <= segments; ++i) {
        if (lua_isfunction(L, 5)) {
            lua_pushvalue(L, 5);
            lua_pushinteger(L, i);

            lua_call(L, 1, 0);

            getGlobalColor(L, &color);
            glColor4f(color.r, color.g, color.b, color.a);
        }

        glVertex2f(cx + x, cy + y);

        float newX = cx * c - cy * s;
        float newY = cx * s + cy * c;
        cx = newX;
        cy = newY;
    }
}

int drawLine(lua_State *L) {
    float x1 = lua_tonumber(L, 1);
    float y1 = lua_tonumber(L, 2);
    float x2 = lua_tonumber(L, 3);
    float y2 = lua_tonumber(L, 4);
    float width = luaL_optnumber(L, 5, 1.0f);

    struct color color;
    getGlobalColor(L, &color);
    
    glColor4f(color.r, color.g, color.b, color.a);
    glLineWidth(width);

    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();

    return 0;
}

int drawRect(lua_State *L) {
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);
    float width = lua_tonumber(L, 3);
    float height = lua_tonumber(L, 4);

    struct color color;
    getGlobalColor(L, &color);
    
    glColor4f(color.r, color.g, color.b, color.a);

    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    return 0;
}

int drawCircle(lua_State *L) {
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);
    float radius = lua_tonumber(L, 3);
    int segments = luaL_optinteger(L, 4, 10);
    
    glBegin(GL_LINE_LOOP);
    circle(x, y, radius, segments, L);
    glEnd();

    return 0;
}

int drawFilledCircle(lua_State *L) {
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);
    float radius = lua_tonumber(L, 3);
    int segments = luaL_optinteger(L, 4, 10);

    glBegin(GL_POLYGON);
    glVertex2f(x, y); //bad color, fix later
    circle(x, y, radius, segments, L);
    glEnd();

    return 0;
}

int drawPoly(lua_State *L) {
    int vertexes = lua_rawlen(L, 1);
    struct color color;

    if (!lua_isfunction(L, 2)) {
        getGlobalColor(L, &color);
        glColor4f(color.r, color.g, color.b, color.a);
    }

    glBegin(GL_POLYGON);

    for (int i = 1; i <= vertexes; i++) {
        lua_rawgeti(L, 1, i);

        lua_rawgeti(L, -1, 1);
        lua_rawgeti(L, -2, 2);
        lua_rawgeti(L, -3, 3);
        
        float x = lua_tonumber(L, -3);
        float y = lua_tonumber(L, -2);
        float z = lua_tonumber(L, -1);

        if (lua_isfunction(L, 2)) {
            lua_pushvalue(L, 2);
            lua_pushinteger(L, i);

            lua_call(L, 1, 0);

            getGlobalColor(L, &color);
            glColor4f(color.r, color.g, color.b, color.a);
        }

        glVertex3f(x, y, z);

        lua_pop(L, 4);
    }

    glEnd();

    return 0;
}

int draw3DLine(lua_State *L) {
    //float vec1 = lua_tonumber(L, 1);
    //float vec2 = lua_tonumber(L, 2);
    //float width = luaL_optnumber(L, 5, 1.0f);
//
    //struct color color;
    //getGlobalColor(L, &color);
    //
    //glColor4f(color.r, color.g, color.b, color.a);
    //glLineWidth(width);
//
    //glBegin(GL_LINES);
    //glVertex2f(x1, y1);
    //glVertex2f(x2, y2);
    //glEnd();
//
    //return 0;
}

int drawVertex(lua_State *L) {
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);
    float z = lua_tonumber(L, 3);

    struct color color;
    getGlobalColor(L, &color);
    
    glColor4f(color.r, color.g, color.b, color.a);

    glVertex3f(x, y, z);

    return 0;
}