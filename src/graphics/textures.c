#include <SDL.h>
#include <SDL_opengl.h>
#include "render.h"
#include "../modules/render.h"
#include "../vanir.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../stb_image.h"

#define targetEnums 11

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

// texture methods ↓↓↓ texture methods ///
int loadImage(lua_State *L) {
    struct texture **texture = (struct texture **)luaL_checkudata(L, 1, "texture");
    const char* path = luaL_checkstring(L, 2);

    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    
    if (!data) {
        throw("Texture", (*texture)->name, stbi_failure_reason());
        
        return 0;
    }

    (*texture)->data = data;
    (*texture)->width = width;
    (*texture)->height = height;
    (*texture)->channels = channels;

    GLenum format;

    switch(channels) {
        case 3:
            format = GL_RGB; break;
        case 4:
            format = GL_RGBA; break;
        default:
            stbi_image_free(data);
        
            throw("Texture", (*texture)->name, "Unsupported number of channels");
            
            return 0;
    }

    glBindTexture(GL_TEXTURE_2D, (*texture)->id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    

    //stbi_image_free(data);

    return 0;
}
// texture methods ↑↑↑ texture methods ///

static const luaL_Reg textureMethods[] = {
    {"loadImage", loadImage},
    {NULL, NULL}
};

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
    texture->data = NULL;

    struct texture **temp = (struct texture **)realloc(texturePool.textures, (texturePool.count + 1) * sizeof(struct texture *));

    if (temp) {
        texturePool.textures = temp;
        texturePool.textures[texturePool.count] = texture;
        texturePool.count += 1;
    } else {
        throw("Texture", name, "Memory allocation error");

        free(texture);

        return 0;
    }

    struct texture **udata = (struct texture **)lua_newuserdata(L, sizeof(struct texture *));
    *udata = texture;

    addMethods(L, "texture", textureMethods);

    return 1;
}

int destroyTexture(lua_State *L) {
    const char* name = luaL_checkstring(L, 1);

    struct texture *texture = findTexture(&texturePool, name);

    glDeleteTextures(1, &texture->id);

    return 0;
}

int drawTexture(lua_State *L) {
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);
    float width = lua_tonumber(L, 3);
    float height = lua_tonumber(L, 4);

    //struct color color;
    //getGlobalColor(L, &color);
    
    //glColor4f(color.r, color.g, color.b, color.a);
    
    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x, y);
    glTexCoord2f(1, 0); glVertex2f(x + width, y);
    glTexCoord2f(1, 1); glVertex2f(x + width, y + height);
    glTexCoord2f(0, 1); glVertex2f(x, y + height);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    return 0;
}