#include "lua_config.h"

#include "vanir.h"
#include "enums.h"
#include "types.h"

#include "modules/testfunc.h"
#include "modules/render.h"
#include "modules/windows.h"
#include "modules/hooks.h"
#include "modules/input.h"
#include "modules/timer.h"

#include <string.h>
#include <stdlib.h>

extern struct windowPool windowPool;

/* ↓ helper functions ↓ */
void setFieldNumber(lua_State *L, const char *key, float data) {
    lua_pushnumber(L, data);
    lua_setfield(L, -2, key);
}

void registerGlobals(lua_State* L, const luaL_Reg* funcs) {
    for (; funcs->name != NULL; ++funcs) {
        lua_pushcfunction(L, funcs->func);
        lua_call(L, 0, 1);
        lua_setglobal(L, funcs->name);
    }
}
/* ↑ helper functions ↑ */

int quit(lua_State *L) {
    /* ↓ close any in-flight frame on every window; ends pass encoder, releases view/texture ↓ */
    for (int i = 0; i < windowPool.count; ++i) {
        struct glfwWindow *w = windowPool.windows[i];
        
        releaseFrame(w);
    }

    /* ↓ destroy per-window gpu resources; pipeline → surface unconfigure → surface release → glfw window ↓ */
    for (int i = 0; i < windowPool.count; ++i) {
        struct glfwWindow *w = windowPool.windows[i];
        
        destroyPipeline(w->pipeline);
        
        w->pipeline = NULL;
        
        if (w->surface) {
            wgpuSurfaceUnconfigure(w->surface);  // required before release on wgpu-native
            wgpuSurfaceRelease(w->surface);
            
            w->surface = NULL;
        }

        if (w->window) {
            glfwDestroyWindow(w->window);
            
            w->window = NULL;
        }
        
        free(w);
    }

    free(windowPool.windows);
    
    windowPool.windows = NULL;
    windowPool.count   = 0;

    /* ↓ release shared GPU context/glfw/and lua; device → adapter → instance ↓ */
    vanirGPUDestroy();
    glfwTerminate();
    lua_close(L);
    
    exit(0);
}

/*
int requiredir(lua_State *L) {
    const char *rel = luaL_checkstring(L, 1);

    // Build the absolute path.
    char cwd[PATH_MAX];

    if (!getcwd(cwd, sizeof(cwd))) {
        return luaL_error(L, "requiredir: getcwd failed");
    }

    char full[PATH_MAX];

    snprintf(full, sizeof(full), "%s/%s", cwd, rel);

    DIR *dir = opendir(full);

    if (!dir) {
        return luaL_error(L, "requiredir: cannot open '%s'", full);
    }

    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;

        // Skip hidden files and . / ..
        if (name[0] == '.') continue;

        // Only load .lua files. Let Lua's require() handle native libs.
        const char *ext = strrchr(name, '.');

        if (!ext || strcmp(ext, ".lua") != 0) continue;

        char path[PATH_MAX];

        snprintf(path, sizeof(path), "%s/%s", full, name);

        // Use luaL_loadfile + lua_pcall instead of luaL_dofile so we can
        // catch errors without aborting the whole directory scan.
        if (luaL_loadfile(L, path) != LUA_OK) {
            // Leave the error on the stack as a warning, then pop and continue.
            fprintf(stderr, "[Vanir] requiredir: load error in '%s': %s\n",
                    name, lua_tostring(L, -1));
            lua_pop(L, 1);
            
            continue;
        }

        // Call the chunk. Files that return a table get that table as the
        // result — we discard it here (requiredir isn't require()), but the
        // file ran and any side-effects (globals, hook registrations) stick.
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            fprintf(stderr, "[Vanir] requiredir: runtime error in '%s': %s\n",
                    name, lua_tostring(L, -1));
            
            lua_pop(L, 1);
        }
    }

    closedir(dir);

    return 0;
}
*/

const luaL_Reg luaVanir[] = {
    {"testt",  l_test},
    /* ↑ random testing, ignore ↑ */

    /* ↓ types ↓ */
    {"Vector", Vector},
    {"Angle", Angle},
    {"Color",  Color},
    
    /* ↓ vanir functions ↓ */
    {"quit", quit},
    //{"requireDir", requireDir}, recoding later today

    {NULL, NULL}
};

const luaL_Reg luaReg[] = {
    /* ↓ modules ↓ */
    {"windows", windowsInit},
    {"render", renderInit},
    {"hook", hooksInit},
    {"input", inputInit},
    {"timer", timerInit},

    /* ↓ enums ↓ */
    {"test", testEnums},

    {NULL, NULL}
};

/// ↓ require("vanir") lua entry ↓ ///
LUALIB_API int luaopen_vanir(lua_State *L) {
#ifdef USE_LUAJIT
    luaL_dostring(L, "require('compat53')");
#endif

    luaL_dofile(L, "preload.lua");

    lua_newtable(L);
    setFieldNumber(L, "r", 255.0f);
    setFieldNumber(L, "g", 255.0f);
    setFieldNumber(L, "b", 255.0f);
    setFieldNumber(L, "a", 255.0f);
    lua_setglobal(L, "_rendercolor");

    lua_newtable(L);
    lua_pushstring(L, "1.0.0");
    lua_setfield(L, -2, "version");

    for (const luaL_Reg *reg = luaVanir; reg->name != NULL && reg->func != NULL; ++reg) {
        lua_pushcfunction(L, reg->func);
        lua_setglobal(L, reg->name);
    }

    registerGlobals(L, luaReg);

    lua_newtable(L);
    lua_pushcfunction(L, hooksRun);
    lua_setfield(L, -2, "run");
    lua_setglobal(L, "hooks");

    return 1;
}
