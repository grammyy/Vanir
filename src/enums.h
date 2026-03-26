#pragma once

#ifndef VANIR_ENUMS_H
#define VANIR_ENUMS_H

#include "lua_config.h"
#include <GLFW/glfw3.h>

typedef struct {
    const char* name;
    int value;
} Enums;

/* ↓ general push command for each enum list ↓ */
void pushEnums(lua_State *L, const Enums *enums);

int testEnums(lua_State *L);
int keyEnums(lua_State *L);
int mouseButtonEnums(lua_State *L);
int keyActionEnums(lua_State *L);
int keyModEnums(lua_State *L);
int cursorModeEnums(lua_State *L);
int cursorShapeEnums(lua_State *L);
int gamepadButtonEnums(lua_State *L);
int gamepadAxisEnums(lua_State *L);

/* ↓ enum declarations ↓ */
extern Enums test[];
extern Enums keys[];
extern Enums mouseButtons[];
extern Enums keyActions[];
extern Enums keyMods[];
extern Enums cursorModes[];
extern Enums cursorShapes[];
extern Enums gamepadButtons[];
extern Enums gamepadAxes[];

#endif
