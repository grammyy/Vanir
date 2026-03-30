#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "../vanir.h"
#include "../types/common.h"
#include "files.h"

/* ↓ file:close() ↓ */
static int fileClose(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "__ptr");

    struct File *f = (struct File *)lua_touserdata(L, -1);

    lua_pop(L, 1);

    if (f && f->handle) {
        fclose(f->handle);

        f->handle = NULL;
    }

    if (f) {
        free(f->path);
        free(f);
    }

    lua_pushnil(L);
    lua_setfield(L, 1, "__ptr");

    return 0;
}

static int toStringFile(lua_State *L) {
    lua_getfield(L, 1, "__ptr");

    struct File *f = (struct File *)lua_touserdata(L, -1);

    lua_pop(L, 1);

    if (!f || !f->handle) {
        lua_pushstring(L, "File: (closed)");
    } else {
        lua_pushfstring(L, "File: %s", f->path ? f->path : "(unknown)");
    }

    return 1;
}

/* ↓ file:flush() ↓ */
static int fileFlush(lua_State *L) {
    struct File *f = getFile(L, 1);

    fflush(f->handle);

    return 0;
}

/* ↓ file:tell() → number ↓ */
static int fileTell(lua_State *L) {
    struct File *f = getFile(L, 1);

    lua_pushinteger(L, (lua_Integer)ftell(f->handle));

    return 1;
}

/* ↓ file:seek(n) → None; seeks to absolute byte position ↓ */
static int fileSeek(lua_State *L) {
    struct File *f = getFile(L, 1);
    long pos = (long)luaL_checkinteger(L, 2);

    fseek(f->handle, pos, SEEK_SET);

    return 0;
}

/* ↓ file:skip(n) → number (resulting position) ↓ */
static int fileSkip(lua_State *L) {
    struct File *f = getFile(L, 1);
    long n = (long)luaL_checkinteger(L, 2);

    fseek(f->handle, n, SEEK_CUR);
    lua_pushinteger(L, (lua_Integer)ftell(f->handle));

    return 1;
}

/* ↓ file:size() → number ↓ */
static int fileSize(lua_State *L) {
    struct File *f   = getFile(L, 1);
    long cur  = ftell(f->handle);

    fseek(f->handle, 0, SEEK_END);

    long size = ftell(f->handle);

    fseek(f->handle, cur, SEEK_SET);
    lua_pushinteger(L, (lua_Integer)size);

    return 1;
}

/* ↓ file:endOfFile() → boolean ↓ */
static int fileEndOfFile(lua_State *L) {
    struct File *f = getFile(L, 1);

    long cur = ftell(f->handle);
    fseek(f->handle, 0, SEEK_END);

    long end = ftell(f->handle);
    fseek(f->handle, cur, SEEK_SET);

    lua_pushboolean(L, cur >= end);

    return 1;
}

/* ↓ file:read(n) → string (raw bytes) ↓ */
static int fileRead(lua_State *L) {
    struct File *f = getFile(L, 1);
    size_t n = (size_t)luaL_checkinteger(L, 2);
    char *buf = (char *)malloc(n);

    if (!buf) {
        throw("File", f->path, "Memory allocation error");
        lua_pushnil(L);

        return 1;
    }

    size_t got = fread(buf, 1, n, f->handle);

    lua_pushlstring(L, buf, got);
    free(buf);

    return 1;
}

/* ↓ file:readLine() → string ↓ */
static int fileReadLine(lua_State *L) {
    struct File *f = getFile(L, 1);
    char buf[4096];

    if (!fgets(buf, sizeof(buf), f->handle)) {
        lua_pushnil(L);

        return 1;
    }

    /* ↓ strip trailing newline ↓ */
    size_t len = strlen(buf);

    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';

        if (len > 0 && buf[len - 1] == '\r')
            buf[--len] = '\0';
    }

    lua_pushlstring(L, buf, len);

    return 1;
}

/* ↓ file:write(str) ↓ */
static int fileWrite(lua_State *L) {
    struct File *f = getFile(L, 1);
    size_t len;
    const char *s = luaL_checklstring(L, 2, &len);

    fwrite(s, 1, len, f->handle);

    return 0;
}

/* --- typed reads --- */

/* ↓ file:readByte() → number (UInt8) ↓ */
static int fileReadByte(lua_State *L) {
    struct File *f = getFile(L, 1);
    uint8_t v;

    fread(&v, 1, 1, f->handle);
    lua_pushinteger(L, (lua_Integer)v);

    return 1;
}

/* ↓ file:readBool() → boolean ↓ */
static int fileReadBool(lua_State *L) {
    struct File *f = getFile(L, 1);
    uint8_t v;

    fread(&v, 1, 1, f->handle);
    lua_pushboolean(L, v != 0);

    return 1;
}

/* ↓ file:readShort() → number (Int16) ↓ */
static int fileReadShort(lua_State *L) {
    struct File *f = getFile(L, 1);
    int16_t v;

    fread(&v, sizeof(v), 1, f->handle);
    lua_pushinteger(L, (lua_Integer)v);

    return 1;
}

/* ↓ file:readUShort() → number (UInt16) ↓ */
static int fileReadUShort(lua_State *L) {
    struct File *f = getFile(L, 1);
    uint16_t v;

    fread(&v, sizeof(v), 1, f->handle);
    lua_pushinteger(L, (lua_Integer)v);

    return 1;
}

/* ↓ file:readLong() → number (Int32) ↓ */
static int fileReadLong(lua_State *L) {
    struct File *f = getFile(L, 1);
    int32_t v;

    fread(&v, sizeof(v), 1, f->handle);
    lua_pushinteger(L, (lua_Integer)v);

    return 1;
}

/* ↓ file:readULong() → number (UInt32) ↓ */
static int fileReadULong(lua_State *L) {
    struct File *f = getFile(L, 1);
    uint32_t v;

    fread(&v, sizeof(v), 1, f->handle);
    lua_pushinteger(L, (lua_Integer)v);

    return 1;
}

/* ↓ file:readFloat() → number (Float32) ↓ */
static int fileReadFloat(lua_State *L) {
    struct File *f = getFile(L, 1);
    float v;

    fread(&v, sizeof(v), 1, f->handle);
    lua_pushnumber(L, (lua_Number)v);

    return 1;
}

/* ↓ file:readDouble() → number (Float64) ↓ */
static int fileReadDouble(lua_State *L) {
    struct File *f = getFile(L, 1);
    double v;

    fread(&v, sizeof(v), 1, f->handle);
    lua_pushnumber(L, (lua_Number)v);

    return 1;
}

/* ↓ file:readUInt64() → string (decimal) ↓ */
static int fileReadUInt64(lua_State *L) {
    struct File *f = getFile(L, 1);
    uint64_t v;
    char buf[22];

    fread(&v, sizeof(v), 1, f->handle);
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
    lua_pushstring(L, buf);

    return 1;
}

/* --- typed writes --- */

/* ↓ file:writeByte(x) ↓ */
static int fileWriteByte(lua_State *L) {
    struct File *f = getFile(L, 1);
    uint8_t v = (uint8_t)luaL_checkinteger(L, 2);

    fwrite(&v, 1, 1, f->handle);

    return 0;
}

/* ↓ file:writeBool(x) ↓ */
static int fileWriteBool(lua_State *L) {
    struct File *f = getFile(L, 1);
    uint8_t v = (uint8_t)(lua_toboolean(L, 2) ? 1 : 0);

    fwrite(&v, 1, 1, f->handle);

    return 0;
}

/* ↓ file:writeShort(x) ↓ */
static int fileWriteShort(lua_State *L) {
    struct File *f = getFile(L, 1);
    int16_t v = (int16_t)luaL_checkinteger(L, 2);

    fwrite(&v, sizeof(v), 1, f->handle);

    return 0;
}

/* ↓ file:writeUShort(x) ↓ */
static int fileWriteUShort(lua_State *L) {
    struct File *f = getFile(L, 1);
    uint16_t v = (uint16_t)luaL_checkinteger(L, 2);

    fwrite(&v, sizeof(v), 1, f->handle);

    return 0;
}

/* ↓ file:writeLong(x) ↓ */
static int fileWriteLong(lua_State *L) {
    struct File *f = getFile(L, 1);
    int32_t v = (int32_t)luaL_checkinteger(L, 2);

    fwrite(&v, sizeof(v), 1, f->handle);

    return 0;
}

/* ↓ file:writeULong(x) ↓ */
static int fileWriteULong(lua_State *L) {
    struct File *f = getFile(L, 1);
    uint32_t v = (uint32_t)luaL_checkinteger(L, 2);

    fwrite(&v, sizeof(v), 1, f->handle);

    return 0;
}

/* ↓ file:writeFloat(x) ↓ */
static int fileWriteFloat(lua_State *L) {
    struct File *f = getFile(L, 1);
    float v = (float)luaL_checknumber(L, 2);

    fwrite(&v, sizeof(v), 1, f->handle);

    return 0;
}

/* ↓ file:writeDouble(x) ↓ */
static int fileWriteDouble(lua_State *L) {
    struct File *f = getFile(L, 1);
    double v = (double)luaL_checknumber(L, 2);

    fwrite(&v, sizeof(v), 1, f->handle);

    return 0;
}

/* ↓ file:writeUInt64(str) — decimal string ↓ */
static int fileWriteUInt64(lua_State *L) {
    struct File *f  = getFile(L, 1);
    const char  *s  = luaL_checkstring(L, 2);
    uint64_t     v  = (uint64_t)strtoull(s, NULL, 10);

    fwrite(&v, sizeof(v), 1, f->handle);

    return 0;
}

/* ↓ exported so types.c can reference them when building the metatable ↓ */
const luaL_Reg fileMethods[] = {
    /* ↓ navigation ↓ */
    {"tell",        fileTell},
    {"seek",        fileSeek},
    {"skip",        fileSkip},
    {"size",        fileSize},
    {"endOfFile",   fileEndOfFile},
    {"flush",       fileFlush},
    {"close",       fileClose},

    /* ↓ raw I/O ↓ */
    {"read",        fileRead},
    {"readLine",    fileReadLine},
    {"write",       fileWrite},

    /* ↓ typed reads ↓ */
    {"readByte",    fileReadByte},
    {"readBool",    fileReadBool},
    {"readShort",   fileReadShort},
    {"readUShort",  fileReadUShort},
    {"readLong",    fileReadLong},
    {"readULong",   fileReadULong},
    {"readFloat",   fileReadFloat},
    {"readDouble",  fileReadDouble},
    {"readUInt64",  fileReadUInt64},

    /* ↓ typed writes ↓ */
    {"writeByte",   fileWriteByte},
    {"writeBool",   fileWriteBool},
    {"writeShort",  fileWriteShort},
    {"writeUShort", fileWriteUShort},
    {"writeLong",   fileWriteLong},
    {"writeULong",  fileWriteULong},
    {"writeFloat",  fileWriteFloat},
    {"writeDouble", fileWriteDouble},
    {"writeUInt64", fileWriteUInt64},

    {NULL, NULL}
};

const luaL_Reg fileMeta[] = {
    {"__tostring", toStringFile},
    {"__gc",       fileClose},

    {NULL, NULL}
};

/* ↓ files.open(path [, mode]) → File | nil ↓ */
/* ↓ mode defaults to "rb"; use "wb" / "r+b" etc for write / read-write ↓ */
static int filesOpen(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const char *mode = luaL_optstring(L, 2, "rb");

    FILE *handle = fopen(path, mode);

    if (!handle) {
        throw("File", path, "Could not open file");
        lua_pushnil(L);

        return 1;
    }

    pushFile(L, handle, path);

    return 1;
}

/* ↓ files.exists(path) → boolean ↓ */
static int filesExists(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    FILE *f = fopen(path, "rb");

    if (f) {
        fclose(f);
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

/* ↓ files.delete(path) → boolean ↓ */
static int filesDelete(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    lua_pushboolean(L, remove(path) == 0);

    return 1;
}

/* ↓ files.rename(from, to) → boolean ↓ */
static int filesRename(lua_State *L) {
    const char *from = luaL_checkstring(L, 1);
    const char *to   = luaL_checkstring(L, 2);

    lua_pushboolean(L, rename(from, to) == 0);

    return 1;
}

static const luaL_Reg luaFiles[] = {
    {"open",   filesOpen},
    {"exists", filesExists},
    {"delete", filesDelete},
    {"rename", filesRename},

    {NULL, NULL}
};

int filesInit(lua_State *L) {
    luaL_newlib(L, luaFiles);

    return 1;
}