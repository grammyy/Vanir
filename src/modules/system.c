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
    #include <locale.h>
    #include <time.h>
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
        
        if (!user) 
            user = getenv("LOGNAME");

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

/* ↓ logical CPU count ↓ */
int getCPUCount(lua_State *L) {
    #ifdef _WIN32
        SYSTEM_INFO si;

        GetSystemInfo(&si);
        lua_pushinteger(L, (lua_Integer)si.dwNumberOfProcessors);
    #else
        long n = sysconf(_SC_NPROCESSORS_ONLN);

        lua_pushinteger(L, n > 0 ? (lua_Integer)n : 1);
    #endif

    return 1;
}

/* ↓ total physical RAM in bytes ↓ */
int getTotalRAM(lua_State *L) {
    #ifdef _WIN32
        MEMORYSTATUSEX ms;
        ms.dwLength = sizeof(ms);

        GlobalMemoryStatusEx(&ms);
        lua_pushnumber(L, (lua_Number)ms.ullTotalPhys);
    #else
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);

        if (pages > 0 && page_size > 0)
            lua_pushnumber(L, (lua_Number)((unsigned long long)pages * (unsigned long long)page_size));
        else
            lua_pushnil(L);
    #endif

    return 1;
}

/* ↓ absolute path to the running executable ↓ */
int getExecutablePath(lua_State *L) {
    #ifdef _WIN32
        char buf[MAX_PATH];
        DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
        
        if (len > 0)
            lua_pushstring(L, buf);
        else
            lua_pushnil(L);
    #else
        char buf[4096];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        
        if (len > 0) {
            buf[len] = '\0';
            lua_pushstring(L, buf);
        } else {
            lua_pushnil(L);
        }
    #endif

    return 1;
}

/* ↓ suspend the calling thread for ms milliseconds ↓ */
int systemSleep(lua_State *L) {
    lua_Integer ms = luaL_checkinteger(L, 1);

    if (ms < 0) 
        ms = 0;

    #ifdef _WIN32
        Sleep((DWORD)ms);
    #else
        struct timespec ts;
        ts.tv_sec  = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000000L;

        nanosleep(&ts, NULL);
    #endif

    return 0;
}

/* ↓ terminate the process cleanly with the given exit code ↓ */
int systemExit(lua_State *L) {
    int code = (int)luaL_optinteger(L, 1, 0);

    exit(code);

    return 0; /* unreachable */
}

/* ↓ IETF locale tag for the current user (e.g. "en-US") ↓ */
int getLocale(lua_State *L) {
    #ifdef _WIN32
        wchar_t wbuf[LOCALE_NAME_MAX_LENGTH];

        if (GetUserDefaultLocaleName(wbuf, LOCALE_NAME_MAX_LENGTH)) {
            int len = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);

            if (len > 0) {
                char *buf = (char *)malloc(len);

                if (buf) {
                    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, len, NULL, NULL);
                    lua_pushstring(L, buf);
                    free(buf);
                    return 1;
                }
            }
        }

        lua_pushstring(L, "en-US");
    #else
        const char *loc = setlocale(LC_ALL, NULL);

        if (!loc || loc[0] == '\0' || strcmp(loc, "C") == 0)
            loc = getenv("LANG");

        lua_pushstring(L, loc ? loc : "en");
    #endif

    return 1;
}

const luaL_Reg luaSystem[] = {
    {"getUsername",      getUsername},
    {"getOS",            getOS},
    {"getSystemInfo",    getSystemInfo},
    {"getMonitors",      getMonitors},
    {"getMonitorCount",  getMonitorCount},
    {"getScreenSize",    getScreenSize},
    {"getClipboard",     getClipboard},
    {"setClipboard",     setClipboard},
    {"getTime",          getTime},
    {"getEnv",           getEnv},
    {"getCPUCount",      getCPUCount},
    {"getTotalRAM",      getTotalRAM},
    {"getExecutablePath",getExecutablePath},
    {"sleep",            systemSleep},
    {"exit",             systemExit},
    {"getLocale",        getLocale},

    {NULL, NULL}
};

int systemInit(lua_State *L) {
    luaL_newlib(L, luaSystem);
    
    return 1;
}