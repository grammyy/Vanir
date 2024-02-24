#include <SDL.h>
#include <SDL_opengl.h>
#include "render.h"
#include "../modules/render.h"
#include "../vanir.h"

#define targetEnums 11

const GLenum blendLookup[blendEnums] = {
    GL_TEXTURE_1D,
    GL_TEXTURE_2D,
    GL_TEXTURE_3D,
    GL_TEXTURE_1D_ARRAY,
    GL_TEXTURE_2D_ARRAY,
    GL_TEXTURE_RECTANGLE,
    GL_TEXTURE_CUBE_MAP,
    GL_TEXTURE_CUBE_MAP_ARRAY,
    GL_TEXTURE_BUFFER,
    GL_TEXTURE_2D_MULTISAMPLE,
    GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
};

int selectTexture(lua_State *L) {
    int target = luaL_checkinteger(L, 1);
    int id = luaL_checkinteger(L, 2);

    glBindTexture(targetLookup[target], id);

    return 0;
}