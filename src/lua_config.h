#pragma once

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

/* ↓ luajit or lua 5.1: define lua_rawlen as lua_objlen ↓ */
#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
  #define lua_rawlen(L, i) lua_objlen(L, i)
#endif