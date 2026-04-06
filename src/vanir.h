#ifndef VANIR_H
#define VANIR_H

#define VANIR_VERSION "v3.3.2"

#include "lua_config.h"
#include "types/common.h"

#include <stdio.h>
#include <stdarg.h>

/* ↓ compatibility shim ↓ */
#ifndef luaL_reg
  #define luaL_reg luaL_Reg
#endif

/* ↓ require("vanir") lua entry ↓ */
LUALIB_API int luaopen_Vanir(lua_State *L);

void setFieldNumber(lua_State *L, const char *key, float data);

/* ↓ logging ↓ */
#ifdef VANIR_VERBOSE
  #define vanir_log(fmt, ...) fprintf(stderr, "[Vanir] " fmt "\n", ##__VA_ARGS__)
  #define vanir_log_info(fmt, ...) fprintf(stderr, "[Vanir] [INFO] " fmt "\n", ##__VA_ARGS__)
#else
  #define vanir_log(fmt, ...) ((void)0)
  #define vanir_log_info(fmt, ...) ((void)0)
#endif

#include "modules/hooks.h"

/* ↓ error handling (clean) ↓ */
static inline void vanir_throw_impl(
    const char* type,
    const char* name,
    const char* error,
    const char* file,
    int line
) {
    fprintf(stderr, "[Vanir] [ERROR] %s \"%s\": %s (%s:%d)\n", type, name, error, file, line);

    char buf[512];

    snprintf(buf, sizeof(buf), "%s \"%s\": %s", type, name, error);
    fireError(buf);
}

/* ↓ macro just injects file/line ↓ */
#define throw(type, name, error) vanir_throw_impl((type), (name), (error), __FILE__, __LINE__)

/* ↑ logging ↑ */

#endif