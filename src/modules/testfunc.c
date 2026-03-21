#include "testfunc.h"

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include "glfw3webgpu.h"
#include <string.h>

#ifdef _WIN32
  #include <windows.h>

    static void show_alert(const char *msg) {
        MessageBoxA(NULL, msg, "Alert", MB_OK | MB_ICONINFORMATION);
    }
#else
    #include <stdlib.h>
    #include <stdio.h>

    static void show_alert(const char *msg) {
        char cmd[1024];

        snprintf(cmd, sizeof(cmd), "zenity --info --text='%s' 2>/dev/null", msg);

        if (system(cmd) == 0) 
          return;

        snprintf(cmd, sizeof(cmd), "kdialog --msgbox '%s' 2>/dev/null", msg);
        
        if (system(cmd) == 0) 
          return;

        snprintf(cmd, sizeof(cmd), "xmessage -center '%s' 2>/dev/null", msg);

        if (system(cmd) == 0) 
          return;

        fprintf(stderr, "[ALERT] %s\n", msg);
    }
#endif

int l_test(lua_State *L) {
    const char *msg = luaL_optstring(L, 1, "hello from the lua C module!");
    
    show_alert(msg);
    lua_pushboolean(L, 1);
    
    return 1;
}