#ifndef SYSTEM
#define SYSTEM

#include "../vanir.h"

int systemInit(lua_State *L);

int getUsername(lua_State *L);
int getOS(lua_State *L);
int getSystemInfo(lua_State *L);
int getMonitorCount(lua_State *L);
int getMonitors(lua_State *L);
int getScreenSize(lua_State *L);
int getClipboard(lua_State *L);
int setClipboard(lua_State *L);
int getTime(lua_State *L);
int getEnv(lua_State *L);
int getCPUCount(lua_State *L);
int getTotalRAM(lua_State *L);
int getExecutablePath(lua_State *L);
int systemSleep(lua_State *L);
int systemExit(lua_State *L);
int getLocale(lua_State *L);

#endif