#include <SDL.h>
#include <SDL_opengl.h>
#include "render.h"
#include "../modules/render.h"
#include "../vanir.h"

int drawLine(lua_State *L) {
    float x1 = lua_tonumber(L, 1);
    float y1 = lua_tonumber(L, 2);
    float x2 = lua_tonumber(L, 3);
    float y2 = lua_tonumber(L, 4);
    struct color color;

    getGlobalColor(L, &color);
    
    glColor4f(color.r, color.g, color.b, color.a);

    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();

    return 0;
}