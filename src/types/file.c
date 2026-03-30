#include "common.h"
#include "../modules/files.h"

#include <stdlib.h>
#include <string.h>

/* ↓ pull the File ptr from the lua object; errors if dead ↓ */
struct File *getFile(lua_State *L, int idx) {
    luaL_checktype(L, idx, LUA_TTABLE);
    lua_getfield(L, idx, "__ptr");

    struct File *f = (struct File *)lua_touserdata(L, -1);

    lua_pop(L, 1);

    if (!f || !f->handle)
        luaL_error(L, "file is closed");

    return f;
}

/* ↓ push a new File lua object from an already-open FILE* ↓ */
void pushFile(lua_State *L, FILE *handle, const char *path) {
    extern const luaL_Reg fileMethods[];
    extern const luaL_Reg fileMeta[];

    struct File *f = (struct File *)malloc(sizeof(struct File));

    if (!f) {
        throw("File", path, "Memory allocation error");
        lua_pushnil(L);
        return;
    }

    f->handle = handle;
    f->path = path ? strdup(path) : NULL;

    lua_newtable(L);
    lua_pushlightuserdata(L, f);
    lua_setfield(L, -2, "__ptr");
    lua_pushstring(L, path ? path : "");
    lua_setfield(L, -2, "path");

    /* ↓ add methods and metatable ↓ */
    luaL_newmetatable(L, "vanir.File");

    lua_newtable(L);

    for (const luaL_Reg *m = fileMethods; m->name; ++m) {
        lua_pushcfunction(L, m->func);
        lua_setfield(L, -2, m->name);
    }

    lua_setfield(L, -2, "__index");

    for (const luaL_Reg *m = fileMeta; m->name; ++m) {
        lua_pushcfunction(L, m->func);
        lua_setfield(L, -2, m->name);
    }

    lua_setmetatable(L, -2);
}