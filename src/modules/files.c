#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    
    #define mkdir(p, m) _mkdir(p)
#else
    #include <dirent.h>
    #include <unistd.h>
#endif

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

/* ↓ files.read(path) → string | nil; reads entire file as a string ↓ */
static int filesRead(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    FILE *f = fopen(path, "rb");

    if (!f) {
        throw("file.read", path, "Could not open file");
        lua_pushnil(L);

        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)malloc((size_t)size);

    if (!buf) {
        fclose(f);
        throw("file.read", path, "Memory allocation error");
        lua_pushnil(L);

        return 1;
    }

    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);

    lua_pushlstring(L, buf, got);
    free(buf);

    return 1;
}

/* ↓ files.readInGame(path) → string | nil; alias of files.read, provided for GMod API parity ↓ */
static int filesReadInGame(lua_State *L) {
    return filesRead(L);
}

/* ↓ files.asyncRead(path, callback) → nil; reads file asynchronously via a deferred callback ↓ */
/* ↓ in Vanir there is no thread pool yet, so the read happens synchronously on the next call ↓ */
static int filesAsyncRead(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    /* ↓ read the file now and call the callback immediately ↓ */
    FILE *f = fopen(path, "rb");

    lua_pushvalue(L, 2);    /* ↓ push callback ↓ */

    if (!f) {
        lua_pushnil(L);
        lua_pushstring(L, "Could not open file");
        lua_call(L, 2, 0);

        return 0;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)malloc((size_t)size);

    if (!buf) {
        fclose(f);
        lua_pushnil(L);
        lua_pushstring(L, "Memory allocation error");
        lua_call(L, 2, 0);

        return 0;
    }

    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);

    lua_pushlstring(L, buf, got);
    free(buf);
    lua_pushnil(L);

    lua_call(L, 2, 0);  /* ↓ callback(content, err) ↓ */

    return 0;
}

/* ↓ files.write(path, content) → boolean ↓ */
static int filesWrite(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    size_t len;
    const char *content = luaL_checklstring(L, 2, &len);

    FILE *f = fopen(path, "wb");

    if (!f) {
        lua_pushboolean(L, 0);

        return 1;
    }

    fwrite(content, 1, len, f);
    fclose(f);

    lua_pushboolean(L, 1);

    return 1;
}

/* ↓ files.append(path, content) → boolean; appends to a file, creating it if needed ↓ */
static int filesAppend(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    size_t len;
    const char *content = luaL_checklstring(L, 2, &len);

    FILE *f = fopen(path, "ab");

    if (!f) {
        lua_pushboolean(L, 0);

        return 1;
    }

    fwrite(content, 1, len, f);
    fclose(f);

    lua_pushboolean(L, 1);

    return 1;
}

/* ↓ helper: build a temp path; Vanir temp files live next to the binary ↓ */
static void buildTempPath(char *out, size_t outSize, const char *name) {
    snprintf(out, outSize, "temp/%s", name);
}

/* ↓ files.readTemp(name) → string | nil ↓ */
static int filesReadTemp(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    char path[4096];

    buildTempPath(path, sizeof(path), name);

    lua_pushstring(L, path);
    lua_replace(L, 1);

    return filesRead(L);
}

/* ↓ files.writeTemp(name, content) → boolean ↓ */
static int filesWriteTemp(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    size_t len;
    const char *content = luaL_checklstring(L, 2, &len);

    char path[4096];
    buildTempPath(path, sizeof(path), name);

    /* ↓ ensure temp/ directory exists ↓ */
    #ifdef _WIN32
        _mkdir("temp");
    #else
        mkdir("temp", 0755);
    #endif

    FILE *f = fopen(path, "wb");

    if (!f) {
        lua_pushboolean(L, 0);

        return 1;
    }

    fwrite(content, 1, len, f);
    fclose(f);

    lua_pushboolean(L, 1);

    return 1;
}

/* ↓ files.existsTemp(name) → boolean ↓ */
static int filesExistsTemp(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    char path[4096];

    buildTempPath(path, sizeof(path), name);

    FILE *f = fopen(path, "rb");

    if (f) {
        fclose(f);
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

/* ↓ files.deleteTemp(name) → boolean ↓ */
static int filesDeleteTemp(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    char path[4096];

    buildTempPath(path, sizeof(path), name);

    lua_pushboolean(L, remove(path) == 0);

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

/* ↓ files.existsInGame(path) → boolean; alias of files.exists ↓ */
static int filesExistsInGame(lua_State *L) {
    return filesExists(L);
}

/* ↓ files.isDir(path) → boolean ↓ */
static int filesIsDir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    #ifdef _WIN32
        DWORD attr = GetFileAttributesA(path);
        lua_pushboolean(L, attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
    #else
        struct stat st;
        lua_pushboolean(L, stat(path, &st) == 0 && S_ISDIR(st.st_mode));
    #endif

    return 1;
}

/* ↓ files.delete(path) → boolean ↓ */
static int filesDelete(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    lua_pushboolean(L, remove(path) == 0);

    return 1;
}

/* ↓ files.createDir(path) → boolean ↓ */
static int filesCreateDir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    #ifdef _WIN32
        lua_pushboolean(L, _mkdir(path) == 0 || errno == EEXIST);
    #else
        lua_pushboolean(L, mkdir(path, 0755) == 0 || errno == EEXIST);
    #endif

    return 1;
}

/* ↓ files.rename(from, to) → boolean ↓ */
static int filesRename(lua_State *L) {
    const char *from = luaL_checkstring(L, 1);
    const char *to   = luaL_checkstring(L, 2);

    lua_pushboolean(L, rename(from, to) == 0);

    return 1;
}

/* ↓ files.find(path [, filter]) → table of filenames; non-recursive directory listing ↓ */
/* ↓ filter is a simple suffix string, e.g. ".lua"; nil/empty returns all entries ↓ */
static int filesFind(lua_State *L) {
    const char *path   = luaL_checkstring(L, 1);
    const char *filter = luaL_optstring(L, 2, NULL);

    lua_newtable(L);

    int idx = 1;

    #ifdef _WIN32
        char search[4096];
        snprintf(search, sizeof(search), "%s\\*", path);

        WIN32_FIND_DATAA data;
        HANDLE h = FindFirstFileA(search, &data);

        if (h == INVALID_HANDLE_VALUE)
            return 1;

        do {
            const char *name = data.cFileName;

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;

            if (filter) {
                size_t nlen = strlen(name);
                size_t flen = strlen(filter);

                if (nlen < flen || strcmp(name + nlen - flen, filter) != 0)
                    continue;
            }

            lua_pushstring(L, name);
            lua_rawseti(L, -2, idx++);
        } while (FindNextFileA(h, &data));

        FindClose(h);
    #else
        DIR *dir = opendir(path);

        if (!dir)
            return 1;

        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL) {
            const char *name = ent->d_name;

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;

            if (filter) {
                size_t nlen = strlen(name);
                size_t flen = strlen(filter);

                if (nlen < flen || strcmp(name + nlen - flen, filter) != 0)
                    continue;
            }

            lua_pushstring(L, name);
            lua_rawseti(L, -2, idx++);
        }

        closedir(dir);
    #endif

    return 1;
}

/* ↓ files.findInGame(path [, filter]) → table; alias of files.find ↓ */
static int filesFindInGame(lua_State *L) {
    return filesFind(L);
}

/* ↓ files.time(path) → number; modification time as a Unix timestamp, 0 on error ↓ */
static int filesTime(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    struct stat st;

    if (stat(path, &st) == 0) {
        lua_pushinteger(L, (lua_Integer)st.st_mtime);
    } else {
        lua_pushinteger(L, 0);
    }

    return 1;
}

static const luaL_Reg luaFiles[] = {
    /* ↓ file handle factory ↓ */
    {"open",        filesOpen},

    /* ↓ whole-file helpers ↓ */
    {"read",        filesRead},
    {"readInGame",  filesReadInGame},
    {"asyncRead",   filesAsyncRead},
    {"write",       filesWrite},
    {"append",      filesAppend},

    /* ↓ temp-directory variants ↓ */
    {"readTemp",    filesReadTemp},
    {"writeTemp",   filesWriteTemp},
    {"existsTemp",  filesExistsTemp},
    {"deleteTemp",  filesDeleteTemp},

    /* ↓ filesystem queries ↓ */
    {"exists",      filesExists},
    {"existsInGame", filesExistsInGame},
    {"isDir",       filesIsDir},
    {"delete",      filesDelete},
    {"createDir",   filesCreateDir},
    {"rename",      filesRename},
    {"find",        filesFind},
    {"findInGame",  filesFindInGame},
    {"time",        filesTime},

    {NULL, NULL}
};

int filesInit(lua_State *L) {
    luaL_newlib(L, luaFiles);

    return 1;
}
