#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "../vanir.h"
#include "memory.h"

#ifdef _WIN32
    #include <windows.h>
#endif

/* ↓ cast a lua integer to a pointer ↓ */
static void *toPtr(lua_State *L, int idx) {
    return (void *)(uintptr_t)luaL_checkinteger(L, idx);
}

/* ↓ read helpers — each returns 1 value or errors ↓ */

/* ↓ memory.readByte(addr) → number ↓ */
static int memReadByte(lua_State *L) {
    uint8_t *p = (uint8_t *)toPtr(L, 1);

    lua_pushinteger(L, (lua_Integer)*p);

    return 1;
}

/* ↓ memory.readShort(addr) → number (Int16) ↓ */
static int memReadShort(lua_State *L) {
    int16_t v;

    memcpy(&v, toPtr(L, 1), sizeof(v));
    lua_pushinteger(L, (lua_Integer)v);

    return 1;
}

/* ↓ memory.readUShort(addr) → number (UInt16) ↓ */
static int memReadUShort(lua_State *L) {
    uint16_t v;

    memcpy(&v, toPtr(L, 1), sizeof(v));
    lua_pushinteger(L, (lua_Integer)v);

    return 1;
}

/* ↓ memory.readLong(addr) → number (Int32) ↓ */
static int memReadLong(lua_State *L) {
    int32_t v;

    memcpy(&v, toPtr(L, 1), sizeof(v));
    lua_pushinteger(L, (lua_Integer)v);

    return 1;
}

/* ↓ memory.readULong(addr) → number (UInt32) ↓ */
static int memReadULong(lua_State *L) {
    uint32_t v;

    memcpy(&v, toPtr(L, 1), sizeof(v));
    lua_pushinteger(L, (lua_Integer)v);

    return 1;
}

/* ↓ memory.readFloat(addr) → number (Float32) ↓ */
static int memReadFloat(lua_State *L) {
    float v;

    memcpy(&v, toPtr(L, 1), sizeof(v));
    lua_pushnumber(L, (lua_Number)v);

    return 1;
}

/* ↓ memory.readDouble(addr) → number (Float64) ↓ */
static int memReadDouble(lua_State *L) {
    double v;

    memcpy(&v, toPtr(L, 1), sizeof(v));
    lua_pushnumber(L, (lua_Number)v);

    return 1;
}

/* ↓ memory.readInt64(addr) → string (because lua integers may be 32-bit on some builds) ↓ */
static int memReadInt64(lua_State *L) {
    int64_t v;

    memcpy(&v, toPtr(L, 1), sizeof(v));
    lua_pushinteger(L, (lua_Integer)v);

    return 1;
}

/* ↓ memory.readUInt64(addr) → string (decimal) to avoid sign loss ↓ */
static int memReadUInt64(lua_State *L) {
    uint64_t v;
    char buf[22];

    memcpy(&v, toPtr(L, 1), sizeof(v));
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
    lua_pushstring(L, buf);

    return 1;
}

/* ↓ memory.readBool(addr) → boolean ↓ */
static int memReadBool(lua_State *L) {
    uint8_t v;

    memcpy(&v, toPtr(L, 1), sizeof(v));
    lua_pushboolean(L, v != 0);

    return 1;
}

/* ↓ memory.readString(addr, len) → string ↓ */
static int memReadString(lua_State *L) {
    void   *p   = toPtr(L, 1);
    size_t  len = (size_t)luaL_checkinteger(L, 2);

    lua_pushlstring(L, (const char *)p, len);

    return 1;
}

/* ↓ memory.readBytes(addr, count) → string (raw bytes) ↓ */
static int memReadBytes(lua_State *L) {
    void   *p   = toPtr(L, 1);
    size_t  n   = (size_t)luaL_checkinteger(L, 2);

    lua_pushlstring(L, (const char *)p, n);

    return 1;
}

/* ↓ write helpers ↓ */

/* ↓ memory.writeByte(addr, value) ↓ */
static int memWriteByte(lua_State *L) {
    uint8_t *p = (uint8_t *)toPtr(L, 1);
    uint8_t  v = (uint8_t)luaL_checkinteger(L, 2);

    *p = v;

    return 0;
}

/* ↓ memory.writeShort(addr, value) ↓ */
static int memWriteShort(lua_State *L) {
    int16_t v = (int16_t)luaL_checkinteger(L, 2);

    memcpy(toPtr(L, 1), &v, sizeof(v));

    return 0;
}

/* ↓ memory.writeUShort(addr, value) ↓ */
static int memWriteUShort(lua_State *L) {
    uint16_t v = (uint16_t)luaL_checkinteger(L, 2);

    memcpy(toPtr(L, 1), &v, sizeof(v));

    return 0;
}

/* ↓ memory.writeLong(addr, value) ↓ */
static int memWriteLong(lua_State *L) {
    int32_t v = (int32_t)luaL_checkinteger(L, 2);

    memcpy(toPtr(L, 1), &v, sizeof(v));

    return 0;
}

/* ↓ memory.writeULong(addr, value) ↓ */
static int memWriteULong(lua_State *L) {
    uint32_t v = (uint32_t)luaL_checkinteger(L, 2);

    memcpy(toPtr(L, 1), &v, sizeof(v));

    return 0;
}

/* ↓ memory.writeFloat(addr, value) ↓ */
static int memWriteFloat(lua_State *L) {
    float v = (float)luaL_checknumber(L, 2);

    memcpy(toPtr(L, 1), &v, sizeof(v));

    return 0;
}

/* ↓ memory.writeDouble(addr, value) ↓ */
static int memWriteDouble(lua_State *L) {
    double v = (double)luaL_checknumber(L, 2);

    memcpy(toPtr(L, 1), &v, sizeof(v));

    return 0;
}

/* ↓ memory.writeInt64(addr, value) — takes integer ↓ */
static int memWriteInt64(lua_State *L) {
    int64_t v = (int64_t)luaL_checkinteger(L, 2);

    memcpy(toPtr(L, 1), &v, sizeof(v));

    return 0;
}

/* ↓ memory.writeUInt64(addr, str) — takes decimal string to avoid sign loss ↓ */
static int memWriteUInt64(lua_State *L) {
    const char *s = luaL_checkstring(L, 2);
    uint64_t    v = (uint64_t)strtoull(s, NULL, 10);

    memcpy(toPtr(L, 1), &v, sizeof(v));

    return 0;
}

/* ↓ memory.writeBool(addr, value) ↓ */
static int memWriteBool(lua_State *L) {
    uint8_t v = (uint8_t)(lua_toboolean(L, 2) ? 1 : 0);

    memcpy(toPtr(L, 1), &v, sizeof(v));

    return 0;
}

/* ↓ memory.writeString(addr, str) — writes raw bytes, no null terminator ↓ */
static int memWriteString(lua_State *L) {
    size_t      len;
    const char *s = luaL_checklstring(L, 2, &len);

    memcpy(toPtr(L, 1), s, len);

    return 0;
}

/* ↓ memory.writeBytes(addr, str) — alias for writeString ↓ */
static int memWriteBytes(lua_State *L) {
    return memWriteString(L);
}

/* ↓ memory.getAddr(str) → integer — address of a lua string's internal buffer ↓ */
/* ↓ useful for testing; don't hold across GC ↓ */
static int memGetAddr(lua_State *L) {
    size_t      len;
    const char *s = luaL_checklstring(L, 1, &len);

    (void)len;
    lua_pushinteger(L, (lua_Integer)(uintptr_t)s);

    return 1;
}

/* ↓ memory.toHex(addr) → string — format address as hex string ↓ */
static int memToHex(lua_State *L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    char buf[20];

    snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)addr);
    lua_pushstring(L, buf);

    return 1;
}

#ifdef _WIN32
/* ↓ memory.protect(addr, size, prot) — change memory protection flags (windows) ↓ */
static int memProtect(lua_State *L) {
    void   *p    = toPtr(L, 1);
    size_t  size = (size_t)luaL_checkinteger(L, 2);
    DWORD   prot = (DWORD)luaL_checkinteger(L, 3);
    DWORD   old;

    lua_pushboolean(L, VirtualProtect(p, size, prot, &old) != 0);
    lua_pushinteger(L, (lua_Integer)old);

    return 2;
}

/* ↓ windows memory page protection constants ↓ */
static int memPageConstants(lua_State *L) {
    lua_newtable(L);

    lua_pushinteger(L, PAGE_NOACCESS);          lua_setfield(L, -2, "NOACCESS");
    lua_pushinteger(L, PAGE_READONLY);           lua_setfield(L, -2, "READONLY");
    lua_pushinteger(L, PAGE_READWRITE);          lua_setfield(L, -2, "READWRITE");
    lua_pushinteger(L, PAGE_WRITECOPY);          lua_setfield(L, -2, "WRITECOPY");
    lua_pushinteger(L, PAGE_EXECUTE);            lua_setfield(L, -2, "EXECUTE");
    lua_pushinteger(L, PAGE_EXECUTE_READ);       lua_setfield(L, -2, "EXECUTE_READ");
    lua_pushinteger(L, PAGE_EXECUTE_READWRITE);  lua_setfield(L, -2, "EXECUTE_READWRITE");

    return 1;
}
#endif /* _WIN32 */

static const luaL_Reg luaMemory[] = {
    /* ↓ reads ↓ */
    {"readByte",    memReadByte},
    {"readShort",   memReadShort},
    {"readUShort",  memReadUShort},
    {"readLong",    memReadLong},
    {"readULong",   memReadULong},
    {"readFloat",   memReadFloat},
    {"readDouble",  memReadDouble},
    {"readInt64",   memReadInt64},
    {"readUInt64",  memReadUInt64},
    {"readBool",    memReadBool},
    {"readString",  memReadString},
    {"readBytes",   memReadBytes},

    /* ↓ writes ↓ */
    {"writeByte",   memWriteByte},
    {"writeShort",  memWriteShort},
    {"writeUShort", memWriteUShort},
    {"writeLong",   memWriteLong},
    {"writeULong",  memWriteULong},
    {"writeFloat",  memWriteFloat},
    {"writeDouble", memWriteDouble},
    {"writeInt64",  memWriteInt64},
    {"writeUInt64", memWriteUInt64},
    {"writeBool",   memWriteBool},
    {"writeString", memWriteString},
    {"writeBytes",  memWriteBytes},

    /* ↓ utilities ↓ */
    {"getAddr",     memGetAddr},
    {"toHex",       memToHex},

#ifdef _WIN32
    {"protect",        memProtect},
    {"pageConstants",  memPageConstants},
#endif

    {NULL, NULL}
};

int memoryInit(lua_State *L) {
    luaL_newlib(L, luaMemory);

    return 1;
}
