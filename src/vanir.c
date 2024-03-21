#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <Windows.h>
#include <math.h>
#include <dirent.h>
#include <unistd.h>
#include <SDL.h>
#include <sys/stat.h>
#include "modules/hooks.h"
#include "modules/input.h"
#include "modules/windows.h"
#include "modules/render.h"
#include "modules/system.h"
#include "modules/timer.h"
#include "enums.h"
#include "vanir.h"

void setFieldInt(lua_State *L, const char *key, float data) {
    lua_pushstring(L, key);
    lua_pushinteger(L, data);
    lua_settable(L, -3);
}

void setFieldFloat(lua_State *L, const char *key, float data) {
    lua_pushstring(L, key);
    lua_pushnumber(L, data);
    lua_settable(L, -3);
}

int tostring(lua_State *L) {
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "z");

    const char *x = lua_tostring(L, -3);
    const char *y = lua_tostring(L, -2);
    const char *z = lua_tostring(L, -1);

    char temp[strlen(x) + strlen(y) + strlen(z) + 4];

    strcpy(temp, x);
    strcat(temp, ", ");
    strcat(temp, y);
    strcat(temp, ", ");
    strcat(temp, z);

    lua_pop(L, 3);

    lua_pushstring(L, temp);

    return 1;
}

static const luaL_Reg tableMethods[] = {
    {"tostring", tostring},
    {NULL, NULL}
};

int Vector(lua_State* L) {
    float x = luaL_optnumber(L, 1, 0.0f);
    float y = luaL_optnumber(L, 2, 0.0f);
    float z = luaL_optnumber(L, 3, 0.0f);

    lua_newtable(L);
    setFieldFloat(L, "x", x);
    setFieldFloat(L, "y", y);
    setFieldFloat(L, "z", z);

    addMethods(L, "table", tableMethods);

    return 1;
}

int Angle(lua_State* L) {
    float roll = luaL_optnumber(L, 1, 0.0f);
    float pitch = luaL_optnumber(L, 2, 0.0f);
    float yaw = luaL_optnumber(L, 3, 0.0f);

    lua_newtable(L);
    setFieldFloat(L, "roll", roll);
    setFieldFloat(L, "pitch", pitch);
    setFieldFloat(L, "yaw", yaw);
    
    addMethods(L, "table", tableMethods);

    return 1;
}

int rgbToHSV(lua_State *L) { //test more later
    lua_getfield(L, 1, "r");
    lua_getfield(L, 1, "g");
    lua_getfield(L, 1, "b");
    lua_getfield(L, 1, "a");

    float r = lua_tonumber(L, -4);
    float g = lua_tonumber(L, -3);
    float b = lua_tonumber(L, -2);
    float a = lua_tonumber(L, -1);

    float min, max, delta;
    float h, s, v;

    min = fminf(r, fminf(g, b));
    max = fmaxf(r, fmaxf(g, b));
    v = max;

    if (max != 0)
        s = (max - min) / max;
    else {
        s = 0;
        h = -1; // Undefined
    }

    delta = max - min;
    if (delta == 0) {
        h = 0;
    } else if (r == max) {
        h = (g - b) / delta;
    } else if (g == max) {
        h = 2 + (b - r) / delta;
    } else {
        h = 4 + (r - g) / delta;
    }

    h *= 60;
    if (h < 0)
        h += 360;

    lua_newtable(L);
    setFieldInt(L, "r", h);
    setFieldInt(L, "g", s);
    setFieldInt(L, "b", v);
    setFieldInt(L, "a", a);

    return 1;
}

int hsvToRGB(lua_State *L) {
    lua_getfield(L, 1, "r");
    lua_getfield(L, 1, "g");
    lua_getfield(L, 1, "b");
    lua_getfield(L, 1, "a");

    double h = lua_tonumber(L, -4);
    double s = lua_tonumber(L, -3) / 100.0f;
    double v = lua_tonumber(L, -2) / 100.0f;
    double a = lua_tonumber(L, -1);

    double r, g, b;

    if (s == 0) {
        r = v;
		g = v;
		b = v;
    } else {
        if (h >= 360)
            h = 0;
        else
            h = h / 60.0f;

		int i = (int)trunc(h);
        double f = h - i; // fractional part of h

        double p = v * (1.0f - s);
        double q = v * (1.0f - (s * f));
        double t = v * (1.0f - (s * (1.0f - f)));

        switch (i) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }
    
    lua_newtable(L);
    setFieldInt(L, "r", r * 255);
    setFieldInt(L, "g", g * 255);
    setFieldInt(L, "b", b * 255);
    setFieldInt(L, "a", a);

    return 1;
}

static const luaL_Reg colorMethods[] = {
    {"rgbToHSV", rgbToHSV},
    {"hsvToRGB", hsvToRGB},
    {NULL, NULL}
};

int Color(lua_State* L) {
    float r = luaL_optnumber(L, 1, 255.0f);
    float g = luaL_optnumber(L, 2, 255.0f);
    float b = luaL_optnumber(L, 3, 255.0f);
    float a = luaL_optnumber(L, 4, 255.0f);

    lua_newtable(L);
    setFieldInt(L, "r", r);
    setFieldInt(L, "g", g);
    setFieldInt(L, "b", b);
    setFieldInt(L, "a", a);

    addMethods(L, "color", colorMethods);

    return 1;
}

void addMethods(lua_State* L, const char* name, const luaL_Reg* methods) {
    luaL_newmetatable(L, name);

    lua_pushstring(L, "__index");
    lua_newtable(L);

    for (const luaL_Reg* method = methods; method->name != NULL; ++method) {
        lua_pushcfunction(L, method->func);
        lua_setfield(L, -2, method->name);
    }

    lua_settable(L, -3);

    lua_setmetatable(L, -2);
}

void registerGlobals(lua_State* L, const luaL_reg* funcs) {
    for (; funcs->name != NULL; ++funcs) {
        lua_pushstring(L, funcs->name);

        funcs->func(L);

        lua_setglobal(L, funcs->name);
    }
}

void throw(const char* type, const char* name, const char* error) {
    printf("%s \"%s\" errored with: %s\n", type, name, error);
}

int requiredir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    char cwd[PATH_MAX];
    struct dirent *ent;
    DIR *dir;

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        throw("Startup", "requiredir", "Failed to get the current working directory");
        
        return 0;
    }

    size_t path_length = strlen(path);
    char *full_path = (char *)malloc(strlen(cwd) + path_length + 2);
    
    if (full_path == NULL) {
        throw("Startup", "requiredir", "Memory allocation error.");
        
        return 0;
    }

    snprintf(full_path, strlen(cwd) + path_length + 2, "%s/%s", cwd, path);

    if ((dir = opendir(full_path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            char *file_name = ent->d_name;
            char *file_path = (char *)malloc(strlen(full_path) + strlen(file_name) + 2);
            
            if (file_path == NULL) {
                throw("requiredir", file_name, "Memory allocation error.");
                
                break;
            }
            
            snprintf(file_path, strlen(full_path) + strlen(file_name) + 2, "%s/%s", full_path, file_name);
            
            struct stat file_stat;

            if (stat(file_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
                char *ext = strrchr(file_name, '.');
                
                if (ext != NULL && (strcmp(ext, ".lua") == 0 || strcmp(ext, ".dll") == 0)) {
                    if (luaL_dofile(L, file_path) != LUA_OK) {
                        throw("Startup", "requiredir", lua_tostring(L, -1));

                        lua_pop(L, 1);
                    }
                }
            }

            free(file_path);
        }

        closedir(dir);
    } else {
        throw("requiredir", full_path, "Failed to open directory.");
    }

    free(full_path);

    return 0;
}

int quit(lua_State *L) {
    SDL_Quit();
    lua_close(L);
}

const luaL_Reg luaVanir[] = {
    {"Vector", Vector},
    {"Color", Color},
    {"requiredir", requiredir},
    {"quit", quit},
    {NULL, NULL}
};

const luaL_reg luaReg[] = {
    // ↓ modules ↓ ///
    {"hooks", hooksInit},
    {"input", inputInit},
    {"windows", windowsInit},
    {"render", renderInit},
    {"timer", timerInit},
    {"system", systemInit},

    // ↓ enums ↓ ///
    {"gl", glEnums},
    {"sdl", sdlEnums},
    {NULL, NULL}
};

void vanirInit(lua_State *L) {
    luaL_dofile(L, "preload.lua");

    lua_newtable(L);

    lua_pushnumber(L, 255.0f);
    lua_setfield(L, -2, "r");

    lua_pushnumber(L, 255.0f);
    lua_setfield(L, -2, "g");

    lua_pushnumber(L, 255.0f);
    lua_setfield(L, -2, "b");

    lua_pushnumber(L, 255.0f);
    lua_setfield(L, -2, "a");

    lua_setglobal(L, "_rendercolor");

    registerGlobals(L, luaReg);

    for (const luaL_Reg *reg = luaVanir; reg->name != NULL && reg->func != NULL; ++reg) {
        lua_pushcfunction(L, reg->func);
        lua_setglobal(L, reg->name);
    }
}

__declspec(dllexport) int luaopen_vanir(lua_State * L) {
    vanirInit(L);
    systemInit(L);

    return 0;
}
