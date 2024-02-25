#include <SDL.h>
#include <SDL_opengl.h>
#include "render.h"
#include "../modules/render.h"
#include "../vanir.h"

#define targetEnums 11

struct texturePool {
    struct texture **textures;
    int count;
};

struct texture {
    const char *name;
    GLuint id;
};

struct texturePool texturePool = {NULL, 0};

const GLenum targetLookup[targetEnums] = {
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

struct texture* findTexture(const struct texturePool* pool, const char* name) {
    for (size_t i = 0; i < pool->count; ++i) {
        if (strcmp(pool->textures[i]->name, name) == 0) {
            return pool->textures[i];
        }
    }
    
    return NULL;
}

int selectTexture(lua_State *L) {
    int target = luaL_checkinteger(L, 1);
    const char* name = luaL_checkstring(L, 2);

    struct texture *texture = findTexture(&texturePool, name);
    
    glBindTexture(targetLookup[target], texture->id);

    return 0;
}

int newTexture(lua_State *L) {
    const char* name = luaL_checkstring(L, 1);

    struct texture *texture = malloc(sizeof(struct texture));

    if (!texture) {
        throw("Texture", name, "Memory allocation error");

        return 0;
    }

    GLuint id;
    glGenTextures(1, &id);

    texture->name = name;
    texture->id = id;

    struct texture **temp = (struct texture **)realloc(texturePool.textures, (texturePool.count + 1) * sizeof(struct texture *));

    if (temp != NULL) {
        texturePool.textures = temp;
        texturePool.textures[texturePool.count] = texture;
        texturePool.count += 1;
    } else {
        throw("Texture", name, "Memory allocation error");

        glDeleteTextures(1, &id);

        return 0;
    }

    //maybe push id here to lua, but doesnt seem to return anything

    return 0;
}

int destroyTexture(lua_State *L) {
    const char* name = luaL_checkstring(L, 1);

    struct texture *texture = findTexture(&texturePool, name);

    glDeleteTextures(1, &texture->id);

    return 0;
}