#pragma once

#include "../lua_config.h"

int l_test(lua_State *L);
int l_test_window(lua_State *L);

extern struct glfwWindow *activeWindow;