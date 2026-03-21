#ifndef VANIR_H
#define VANIR_H

#include "lua_config.h"
#include "types.h"

#include <stdio.h>

/* ↓ compatibility shim ↓ */
#ifndef luaL_reg
  #define luaL_reg luaL_Reg
#endif

/* ↓ require("vanir") lua entry ↓ */
LUALIB_API int luaopen_Vanir(lua_State *L);

void setFieldNumber(lua_State *L, const char *key, float data);
void addMethods(lua_State* L, const char* name, const luaL_Reg* methods, const luaL_Reg* meta);

/* ↓ logging ↓ */
#ifdef VANIR_VERBOSE
  #define vanir_log(fmt, ...) fprintf(stderr, "[Vanir] " fmt "\n", ##__VA_ARGS__)
  #define vanir_log_info(fmt, ...) fprintf(stderr, "[Vanir] [INFO] " fmt "\n", ##__VA_ARGS__)
#else
  #define vanir_log(fmt, ...)      ((void)0)
  #define vanir_log_info(fmt, ...) ((void)0)
#endif

#define throw(type, name, error) fprintf(stderr, "[Vanir] [ERROR] %s \"%s\": %s  (%s:%d)\n", (type), (name), (error), __FILE__, __LINE__)
/* ↑ logging ↑ */

#endif