#include "enums.h"

/* ↓ test enums ↓ */
Enums test[] = {
    {"test", 10},
    {NULL, 0}
};

/* ↓ keyboard keys — GLFW key codes ↓ */
Enums keys[] = {
    /* ↓ printable keys ↓ */
    {"SPACE",         GLFW_KEY_SPACE},
    {"APOSTROPHE",    GLFW_KEY_APOSTROPHE},
    {"COMMA",         GLFW_KEY_COMMA},
    {"MINUS",         GLFW_KEY_MINUS},
    {"PERIOD",        GLFW_KEY_PERIOD},
    {"SLASH",         GLFW_KEY_SLASH},
    {"0",             GLFW_KEY_0},
    {"1",             GLFW_KEY_1},
    {"2",             GLFW_KEY_2},
    {"3",             GLFW_KEY_3},
    {"4",             GLFW_KEY_4},
    {"5",             GLFW_KEY_5},
    {"6",             GLFW_KEY_6},
    {"7",             GLFW_KEY_7},
    {"8",             GLFW_KEY_8},
    {"9",             GLFW_KEY_9},
    {"SEMICOLON",     GLFW_KEY_SEMICOLON},
    {"EQUAL",         GLFW_KEY_EQUAL},
    {"A",             GLFW_KEY_A},
    {"B",             GLFW_KEY_B},
    {"C",             GLFW_KEY_C},
    {"D",             GLFW_KEY_D},
    {"E",             GLFW_KEY_E},
    {"F",             GLFW_KEY_F},
    {"G",             GLFW_KEY_G},
    {"H",             GLFW_KEY_H},
    {"I",             GLFW_KEY_I},
    {"J",             GLFW_KEY_J},
    {"K",             GLFW_KEY_K},
    {"L",             GLFW_KEY_L},
    {"M",             GLFW_KEY_M},
    {"N",             GLFW_KEY_N},
    {"O",             GLFW_KEY_O},
    {"P",             GLFW_KEY_P},
    {"Q",             GLFW_KEY_Q},
    {"R",             GLFW_KEY_R},
    {"S",             GLFW_KEY_S},
    {"T",             GLFW_KEY_T},
    {"U",             GLFW_KEY_U},
    {"V",             GLFW_KEY_V},
    {"W",             GLFW_KEY_W},
    {"X",             GLFW_KEY_X},
    {"Y",             GLFW_KEY_Y},
    {"Z",             GLFW_KEY_Z},
    {"LBRACKET",      GLFW_KEY_LEFT_BRACKET},
    {"BACKSLASH",     GLFW_KEY_BACKSLASH},
    {"RBRACKET",      GLFW_KEY_RIGHT_BRACKET},
    {"GRAVE",         GLFW_KEY_GRAVE_ACCENT},

    /* ↓ function keys ↓ */
    {"ESCAPE",        GLFW_KEY_ESCAPE},
    {"ENTER",         GLFW_KEY_ENTER},
    {"TAB",           GLFW_KEY_TAB},
    {"BACKSPACE",     GLFW_KEY_BACKSPACE},
    {"INSERT",        GLFW_KEY_INSERT},
    {"DELETE",        GLFW_KEY_DELETE},
    {"RIGHT",         GLFW_KEY_RIGHT},
    {"LEFT",          GLFW_KEY_LEFT},
    {"DOWN",          GLFW_KEY_DOWN},
    {"UP",            GLFW_KEY_UP},
    {"PAGEUP",        GLFW_KEY_PAGE_UP},
    {"PAGEDOWN",      GLFW_KEY_PAGE_DOWN},
    {"HOME",          GLFW_KEY_HOME},
    {"END",           GLFW_KEY_END},
    {"CAPSLOCK",      GLFW_KEY_CAPS_LOCK},
    {"SCROLLLOCK",    GLFW_KEY_SCROLL_LOCK},
    {"NUMLOCK",       GLFW_KEY_NUM_LOCK},
    {"PRINTSCREEN",   GLFW_KEY_PRINT_SCREEN},
    {"PAUSE",         GLFW_KEY_PAUSE},
    {"F1",            GLFW_KEY_F1},
    {"F2",            GLFW_KEY_F2},
    {"F3",            GLFW_KEY_F3},
    {"F4",            GLFW_KEY_F4},
    {"F5",            GLFW_KEY_F5},
    {"F6",            GLFW_KEY_F6},
    {"F7",            GLFW_KEY_F7},
    {"F8",            GLFW_KEY_F8},
    {"F9",            GLFW_KEY_F9},
    {"F10",           GLFW_KEY_F10},
    {"F11",           GLFW_KEY_F11},
    {"F12",           GLFW_KEY_F12},

    /* ↓ keypad ↓ */
    {"KP0",           GLFW_KEY_KP_0},
    {"KP1",           GLFW_KEY_KP_1},
    {"KP2",           GLFW_KEY_KP_2},
    {"KP3",           GLFW_KEY_KP_3},
    {"KP4",           GLFW_KEY_KP_4},
    {"KP5",           GLFW_KEY_KP_5},
    {"KP6",           GLFW_KEY_KP_6},
    {"KP7",           GLFW_KEY_KP_7},
    {"KP8",           GLFW_KEY_KP_8},
    {"KP9",           GLFW_KEY_KP_9},
    {"KP_DECIMAL",    GLFW_KEY_KP_DECIMAL},
    {"KP_DIVIDE",     GLFW_KEY_KP_DIVIDE},
    {"KP_MULTIPLY",   GLFW_KEY_KP_MULTIPLY},
    {"KP_SUBTRACT",   GLFW_KEY_KP_SUBTRACT},
    {"KP_ADD",        GLFW_KEY_KP_ADD},
    {"KP_ENTER",      GLFW_KEY_KP_ENTER},
    {"KP_EQUAL",      GLFW_KEY_KP_EQUAL},

    /* ↓ modifier keys ↓ */
    {"LSHIFT",        GLFW_KEY_LEFT_SHIFT},
    {"LCTRL",         GLFW_KEY_LEFT_CONTROL},
    {"LALT",          GLFW_KEY_LEFT_ALT},
    {"LSUPER",        GLFW_KEY_LEFT_SUPER},
    {"RSHIFT",        GLFW_KEY_RIGHT_SHIFT},
    {"RCTRL",         GLFW_KEY_RIGHT_CONTROL},
    {"RALT",          GLFW_KEY_RIGHT_ALT},
    {"RSUPER",        GLFW_KEY_RIGHT_SUPER},
    {"MENU",          GLFW_KEY_MENU},

    {NULL, 0}
};

/* ↓ mouse buttons ↓ */
Enums mouseButtons[] = {
    {"LEFT CLICK"  , GLFW_MOUSE_BUTTON_LEFT},
    {"RIGHT CLICK" , GLFW_MOUSE_BUTTON_RIGHT},
    {"MIDDLE CLICK", GLFW_MOUSE_BUTTON_MIDDLE},
    {"4",            GLFW_MOUSE_BUTTON_4},
    {"5",            GLFW_MOUSE_BUTTON_5},
    {"6",            GLFW_MOUSE_BUTTON_6},
    {"7",            GLFW_MOUSE_BUTTON_7},
    {"8",            GLFW_MOUSE_BUTTON_8},

    {NULL, 0}
};

/* ↓ key/button action states ↓ */
Enums keyActions[] = {
    {"RELEASE", GLFW_RELEASE},
    {"PRESS",   GLFW_PRESS},
    {"REPEAT",  GLFW_REPEAT},

    {NULL, 0}
};

/* ↓ modifier key bitmasks ↓ */
Enums keyMods[] = {
    {"SHIFT",    GLFW_MOD_SHIFT},
    {"CTRL",     GLFW_MOD_CONTROL},
    {"ALT",      GLFW_MOD_ALT},
    {"SUPER",    GLFW_MOD_SUPER},
    {"CAPSLOCK", GLFW_MOD_CAPS_LOCK},
    {"NUMLOCK",  GLFW_MOD_NUM_LOCK},

    {NULL, 0}
};

/* ↓ cursor modes ↓ */
Enums cursorModes[] = {
    {"NORMAL",   GLFW_CURSOR_NORMAL},
    {"HIDDEN",   GLFW_CURSOR_HIDDEN},
    {"DISABLED", GLFW_CURSOR_DISABLED},

    {NULL, 0}
};

/* ↓ standard cursor shapes ↓ */
Enums cursorShapes[] = {
    {"ARROW",      GLFW_ARROW_CURSOR},
    {"IBEAM",      GLFW_IBEAM_CURSOR},
    {"CROSSHAIR",  GLFW_CROSSHAIR_CURSOR},
    {"HAND",       GLFW_POINTING_HAND_CURSOR},
    {"RESIZE_EW",  GLFW_RESIZE_EW_CURSOR},
    {"RESIZE_NS",  GLFW_RESIZE_NS_CURSOR},
    {"RESIZE_NWSE",GLFW_RESIZE_NWSE_CURSOR},
    {"RESIZE_NESW",GLFW_RESIZE_NESW_CURSOR},
    {"RESIZE_ALL", GLFW_RESIZE_ALL_CURSOR},
    {"NOT_ALLOWED",GLFW_NOT_ALLOWED_CURSOR},

    {NULL, 0}
};

/* ↓ joystick / gamepad buttons ↓ */
Enums gamepadButtons[] = {
    {"A",           GLFW_GAMEPAD_BUTTON_A},
    {"B",           GLFW_GAMEPAD_BUTTON_B},
    {"X",           GLFW_GAMEPAD_BUTTON_X},
    {"Y",           GLFW_GAMEPAD_BUTTON_Y},
    {"LBUMPER",     GLFW_GAMEPAD_BUTTON_LEFT_BUMPER},
    {"RBUMPER",     GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER},
    {"BACK",        GLFW_GAMEPAD_BUTTON_BACK},
    {"START",       GLFW_GAMEPAD_BUTTON_START},
    {"GUIDE",       GLFW_GAMEPAD_BUTTON_GUIDE},
    {"LTHUMB",      GLFW_GAMEPAD_BUTTON_LEFT_THUMB},
    {"RTHUMB",      GLFW_GAMEPAD_BUTTON_RIGHT_THUMB},
    {"DPAD_UP",     GLFW_GAMEPAD_BUTTON_DPAD_UP},
    {"DPAD_RIGHT",  GLFW_GAMEPAD_BUTTON_DPAD_RIGHT},
    {"DPAD_DOWN",   GLFW_GAMEPAD_BUTTON_DPAD_DOWN},
    {"DPAD_LEFT",   GLFW_GAMEPAD_BUTTON_DPAD_LEFT},

    {NULL, 0}
};

/* ↓ joystick / gamepad axes ↓ */
Enums gamepadAxes[] = {
    {"LEFT_X",        GLFW_GAMEPAD_AXIS_LEFT_X},
    {"LEFT_Y",        GLFW_GAMEPAD_AXIS_LEFT_Y},
    {"RIGHT_X",       GLFW_GAMEPAD_AXIS_RIGHT_X},
    {"RIGHT_Y",       GLFW_GAMEPAD_AXIS_RIGHT_Y},
    {"LEFT_TRIGGER",  GLFW_GAMEPAD_AXIS_LEFT_TRIGGER},
    {"RIGHT_TRIGGER", GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER},

    {NULL, 0}
};

/* ↓ general push command for each enum list ↓ */
void pushEnums(lua_State *L, const Enums *enums) {
    lua_newtable(L);
    
    for (int i = 0; enums[i].name != NULL; ++i) {
        lua_pushstring(L, enums[i].name);
        lua_pushinteger(L, enums[i].value);
        lua_rawset(L, -3);
    }
}

int testEnums(lua_State *L) {
    pushEnums(L, test);

    return 1;
}

int keyEnums(lua_State *L) {
    pushEnums(L, keys);

    return 1;
}

int mouseButtonEnums(lua_State *L) {
    pushEnums(L, mouseButtons);

    return 1;
}

int keyActionEnums(lua_State *L) {
    pushEnums(L, keyActions);

    return 1;
}

int keyModEnums(lua_State *L) {
    pushEnums(L, keyMods);

    return 1;
}

int cursorModeEnums(lua_State *L) {
    pushEnums(L, cursorModes);

    return 1;
}

int cursorShapeEnums(lua_State *L) {
    pushEnums(L, cursorShapes);

    return 1;
}

int gamepadButtonEnums(lua_State *L) {
    pushEnums(L, gamepadButtons);

    return 1;
}

int gamepadAxisEnums(lua_State *L) {
    pushEnums(L, gamepadAxes);

    return 1;
}
