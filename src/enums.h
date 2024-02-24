#ifndef VANIR_ENUMS
#define VANIR_ENUMS

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <SDL.h>
#include <SDL_opengl.h>

typedef struct {
    const char* name;
    int value;
} Enums;

static Enums gl[] = {
    {"points", 9},
    {"line", 8},
    {"lineStrip", 7},
    {"lineLoop", 6},
    {"triangle", 5},
    {"triangleStrip", 4},
    {"triangleFan", 3},
    {"quad", 2},
    {"quadStrip", 1},
    {"polygon", 0},

    {"stencil", 2},
    {"depth", 1},
    {"color", 0},

    {"srcAlphaSaturate", 14},
    {"inverseAlpha", 13},
    {"alpha", 12},
    {"inverseColor", 11},
    {"color", 10},
    {"inverseDstAlpha", 9},
    {"distAlpha", 8},
    {"inverseSrcAlpha", 7},
    {"srcAlpha", 6},
    {"inverseDstColor", 5},
    {"dstColor", 4},
    {"inverseSrcColor", 3},
    {"srcColor", 2},
    {"one", 1},
    {"zero", 0},

    {"blend", 27},
    {"clipDist", 26},
    {"colorLogic", 25},
    {"cullFace", 24},
    {"debug", 23},
    {"debugSync", 22},
    {"depthClamp", 21},
    {"depthTest", 20},
    {"dither", 19},
    {"framebuffer", 18},
    {"lineSmooth", 17},
    {"multisample", 16},
    {"polygonFill", 15},
    {"polygonLine", 14},
    {"polygonPoint", 13},
    {"polygonSmooth", 12},
    {"primitive", 11},
    {"primitiveIndex", 10},
    {"rasterizerDisgard", 9},
    {"alphaCoverage", 8},
    {"AlphaToOne", 7},
    {"sampleCoverage", 6},
    {"sampleShading", 5},
    {"sampleMask", 4},
    {"scissor", 3},
    {"stencil", 2},
    {"texture", 1},
    {"pointSize", 0},
    
    {"shaders", 3},
    {"lines", 2},
    {"polygons", 1},
    {"texture", 0},

    {"fastest", 2},
    {"nicest", 1},
    {"system", 0},
    {NULL, 0}
};

static Enums sdl[] = {
    {"flipped portrait", 4},
    {"portrait", 3},
    {"flipped landscape", 2},
    {"landscape", 1},
    {"unknown", 0},
    
    {"first", 0},
    {"quit", 256},
    {"terminating", 257},
    {"lowMemory", 258},
    {"willEnterBackground", 259},
    {"didEnterBackground", 260},
    {"willEnterForeground", 261},
    {"didEnterForeground", 262},
    {"localeChanged", 263},
    {"display", 336},
    {"window", 512},
    {"system", 513},
    {"keyDown", 768},
    {"keyUp", 769},
    {"textEditing", 770},
    {"textInput", 771},
    {"keyMapChanged", 772},
    {"textEditingExt", 773},
    {"mouseMotion", 1024},
    {"mousePressed", 1025},
    {"mouseReleased", 1026},
    {"mouseWheel", 1027},
    {"joystickAxisMotion", 1536},
    {"joystickBallMotion", 1537},
    {"joystickHatMotion", 1538},
    {"joystickPressed", 1539},
    {"joystickReleased", 1540},
    {"joystickAdded", 1541},
    {"joystickRemoved", 1542},
    {"joystickBatteryUpdated", 1543},
    {"controlleraAxisMotion", 1616},
    {"controllerPressed", 1617},
    {"controllerReleased", 1618},
    {"controllerAdded", 1619},
    {"controllerRemoved", 1620},
    {"controllerMapped", 1621},
    {"controllerPadDown", 1622},
    {"controllerPadMotion", 1623},
    {"controllerPadUp", 1624},
    {"controllerSensorUpdate", 1625},
    {"fingerPressed", 1792},
    {"fingerReleased", 1793},
    {"fingerMotion", 1794},
    {"dollarGesture", 2048},
    {"dollarCord", 2049},
    {"multiGesture", 2050},
    {"clipboardUpdate", 2304},
    {"dropFile", 4096},
    {"dropText", 4097},
    {"dropBegin", 4098},
    {"dropDone", 4099},
    {"audioAdded", 4352},
    {"audioRemoved", 4353},
    {"sensorUpdate", 4608},
    {"rendertargetsReset", 8192},
    {"renderReset", 8193},
    {"pollEnd", 32512},
    {"user", 32768},
    {"last", 65535},
    {NULL, 0}
};

void pushEnums(lua_State *L, const Enums *Enum) {
    lua_newtable(L);

    for (int i = 0; Enum[i].name != NULL; ++i) {
        lua_pushstring(L, Enum[i].name);
        lua_pushinteger(L, Enum[i].value);
        lua_settable(L, -3);
    }
}

int glEnums(lua_State* L) {
    pushEnums(L, gl);

    return 1;
}

int sdlEnums(lua_State* L) {
    lua_newtable(L);

    for (int i = 0; sdl[i].name != NULL; ++i) {
        lua_pushinteger(L, sdl[i].value);
        lua_pushstring(L, sdl[i].name);
        lua_settable(L, -3);
    }

    return 1;
}

#endif