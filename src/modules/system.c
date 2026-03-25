#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../vanir.h"
#include "system.h"

/* ↓ platform includes ↓ */
#ifdef _WIN32
    #include <windows.h>
    #include <lmcons.h>
#else
    #include <unistd.h>
    #include <pwd.h>
    #include <sys/utsname.h>
#endif

/* ↓ current logged-in username ↓ */
int getUsername(lua_State *L) {
#ifdef _WIN32
    char buf[UNLEN + 1];
    DWORD sz = sizeof(buf);
    
    if (GetUserNameA(buf, &sz))
        lua_pushstring(L, buf);
    else
        lua_pushstring(L, "unknown");
#else
    const char *user = getenv("USER");
    
    if (!user) user = getenv("LOGNAME");
    if (!user) {
        struct passwd *pw = getpwuid(getuid());
        user = pw ? pw->pw_name : "unknown";
    }
    
    lua_pushstring(L, user);
#endif

    return 1;
}

/* ↓ OS name string ↓ */
int getOS(lua_State *L) {
#ifdef _WIN32
    lua_pushstring(L, "windows");
#elif defined(__APPLE__)
    lua_pushstring(L, "macos");
#elif defined(__linux__)
    lua_pushstring(L, "linux");
#else
    lua_pushstring(L, "unknown");
#endif

    return 1;
}

/* ↓ kernel/system info table: { sysname, nodename, release, version, machine } ↓ */
int getSystemInfo(lua_State *L) {
    lua_newtable(L);

#ifdef _WIN32
    lua_pushstring(L, "windows");
    lua_setfield(L, -2, "sysname");
    
    char hostname[256] = {0};
    DWORD hsz = sizeof(hostname);
    
    GetComputerNameA(hostname, &hsz);
    lua_pushstring(L, hostname);
    lua_setfield(L, -2, "nodename");
    
    OSVERSIONINFOEXA osv = { sizeof(OSVERSIONINFOEXA) };
    
#pragma warning(suppress: 4996)
    GetVersionExA((LPOSVERSIONINFOA)&osv);
    
    char ver[64];
    snprintf(ver, sizeof(ver), "%lu.%lu.%lu", osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber);
    lua_pushstring(L, ver);
    lua_setfield(L, -2, "release");
    
    lua_pushstring(L, "");
    lua_setfield(L, -2, "version");
    
    lua_pushstring(L, sizeof(void*) == 8 ? "x86_64" : "x86");
    lua_setfield(L, -2, "machine");
#else
    struct utsname u;
    
    if (uname(&u) == 0) {
        lua_pushstring(L, u.sysname);   lua_setfield(L, -2, "sysname");
        lua_pushstring(L, u.nodename);  lua_setfield(L, -2, "nodename");
        lua_pushstring(L, u.release);   lua_setfield(L, -2, "release");
        lua_pushstring(L, u.version);   lua_setfield(L, -2, "version");
        lua_pushstring(L, u.machine);   lua_setfield(L, -2, "machine");
    }
#endif

    return 1;
}

/* ↓ monitor count ↓ */
int getMonitorCount(lua_State *L) {
    int count;
    
    glfwGetMonitors(&count);
    lua_pushinteger(L, count);
    
    return 1;
}

/* ↓ monitor info table array; { width, height, refreshRate, name } per entry ↓ */
int getMonitors(lua_State *L) {
    int count;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    
    lua_newtable(L);
    
    for (int i = 0; i < count; ++i) {
        const GLFWvidmode *mode = glfwGetVideoMode(monitors[i]);
        
        lua_newtable(L);
        lua_pushinteger(L, mode->width);        lua_setfield(L, -2, "width");
        lua_pushinteger(L, mode->height);       lua_setfield(L, -2, "height");
        lua_pushinteger(L, mode->refreshRate);  lua_setfield(L, -2, "refreshRate");
        lua_pushstring(L, glfwGetMonitorName(monitors[i]));
                                                lua_setfield(L, -2, "name");
        lua_rawseti(L, -2, i + 1);
    }
    
    return 1;
}

/* ↓ primary monitor resolution shorthand ↓ */
int getScreenSize(lua_State *L) {
    int count;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    int idx = (int)luaL_optinteger(L, 1, 1) - 1; // ↓ 1-indexed from lua ↓
    
    if (idx < 0 || idx >= count) idx = 0;
    
    const GLFWvidmode *mode = glfwGetVideoMode(monitors[idx]);
    
    lua_pushinteger(L, mode->width);
    lua_pushinteger(L, mode->height);
    
    return 2;
}

/* ↓ clipboard read / write ↓ */
int getClipboard(lua_State *L) {
    const char *text = glfwGetClipboardString(NULL);
    
    if (text)
        lua_pushstring(L, text);
    else
        lua_pushnil(L);
    
    return 1;
}

int setClipboard(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    
    glfwSetClipboardString(NULL, text);
    
    return 0;
}

/* ↓ wall clock time in seconds (double) ↓ */
int getTime(lua_State *L) {
    lua_pushnumber(L, glfwGetTime());
    
    return 1;
}

/* ↓ environment variable ↓ */
int getEnv(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);
    const char *val = getenv(key);
    
    if (val)
        lua_pushstring(L, val);
    else
        lua_pushnil(L);
    
    return 1;
}

const luaL_Reg luaSystem[] = {
    {"getUsername",   getUsername},
    {"getOS",         getOS},
    {"getSystemInfo", getSystemInfo},
    {"getMonitors",   getMonitors},
    {"getMonitorCount", getMonitorCount},
    {"getScreenSize", getScreenSize},
    {"getClipboard",  getClipboard},
    {"setClipboard",  setClipboard},
    {"getTime",       getTime},
    {"getEnv",        getEnv},

    {NULL, NULL}
};

int systemInit(lua_State *L) {
    luaL_newlib(L, luaSystem);
    
    return 1;
}
