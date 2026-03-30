#include "lua_config.h"

#include "vanir.h"
#include "enums.h"
#include "types/common.h"

#include "modules/testfunc.h"
#include "modules/render.h"
#include "modules/windows.h"
#include "modules/hooks.h"
#include "modules/input.h"
#include "modules/timer.h"
#include "modules/system.h"
#include "modules/font.h"
#include "modules/memory.h"
#include "modules/files.h"
#include "graphics/rendertarget.h"
#include "graphics/textures.h"
#include "graphics/shader.h"

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
        
        /* ↓ destroy pipeline first (uses GPU device) ↓ */
        if (w->pipeline) {
            destroyPipeline(w->pipeline);

            w->pipeline = NULL;
        }
        
        /* ↓ unconfigure & release surface ↓ */
        if (w->surface) {
            wgpuSurfaceUnconfigure(w->surface);  // required before release on wgpu-native
            wgpuSurfaceRelease(w->surface);

            w->surface = NULL;
        }

        /* ↓ destroy GLFW window ↓ */
        if (w->window) {
            glfwDestroyWindow(w->window);

            w->window = NULL;
        }

        free(w);
    }

    free(windowPool.windows);
    
    windowPool.windows = NULL;
    windowPool.count   = 0;

    /* ↓ release font and render target resources before gpu context goes away ↓ */
    //destroyAllFonts();
    destroyAllRenderTargets();
    destroyAllTextures();
    destroyAllShaders();

    /* ↓ release shared GPU context/glfw/and lua; device → adapter → instance ↓ */
    vanirGPUDestroy();  /* device destroyed last */
    glfwTerminate();    /* terminate GLFW after all windows gone */
    lua_close(L);       /* close Lua state last */

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
    {"Color", Color},
    {"Quaternion", Quaternion},
    {"Matrix", Matrix},
    
    /* ↓ vanir functions ↓ */
    {"quit", quit},
    //{"requireDir", requireDir}, recoding later today

    {NULL, NULL}
};

const luaL_Reg luaReg[] = {
    /* ↓ modules ↓ */
    {"windows",  windowsInit},
    {"render",   renderInit},
    {"hook",     hooksInit},
    {"input",    inputInit},
    {"timer",    timerInit},
    {"system",   systemInit},
    {"font",     fontInit},
    {"textures", texturesInit},
    {"shader",   shaderInit},
    {"memory",   memoryInit},
    {"files",    filesInit},

    /* ↓ enums ↓ */
    {"test",         testEnums},
    {"KEY",          keyEnums},
    {"MOUSE",        mouseButtonEnums},
    {"KEY_ACTION",   keyActionEnums},
    {"KEY_MOD",      keyModEnums},
    {"CURSOR_MODE",  cursorModeEnums},
    {"CURSOR_SHAPE", cursorShapeEnums},
    {"GAMEPAD",      gamepadButtonEnums},
    {"GAMEPAD_AXIS", gamepadAxisEnums},

    {NULL, NULL}
};

/// ↓ require("vanir") lua entry ↓ ///
LUALIB_API int luaopen_vanir(lua_State *L) {
    if (luaL_loadfile(L, "preload.lua") == LUA_OK) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            fprintf(stderr, "[Vanir] preload.lua error: %s\n", lua_tostring(L, -1));

            lua_pop(L, 1);
        }
    } else {
        lua_pop(L, 1);
    }

    lua_newtable(L);
    lua_pushstring(L, VANIR_VERSION);
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