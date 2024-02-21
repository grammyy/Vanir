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
    {"points", 10},
    {"line", 9},
    {"lineStrip", 8},
    {"lineLoop", 7},
    {"triangle", 6},
    {"triangleStrip", 5},
    {"triangleFan", 4},
    {"quad", 3},
    {"quadStrip", 2},
    {"polygon", 1},

    {"stencil", 3},
    {"depth", 2},
    {"color", 1},

    {"srcAlphaSaturate", 15},
    {"inverseAlpha", 14},
    {"alpha", 13},
    {"inverseColor", 12},
    {"color", 11},
    {"inverseDstAlpha", 10},
    {"distAlpha", 9},
    {"inverseSrcAlpha", 8},
    {"srcAlpha", 7},
    {"inverseDstColor", 6},
    {"dstColor", 5},
    {"inverseSrcColor", 4},
    {"srcColor", 3},
    {"one", 2},
    {"zero", 1},

    {"blend", 28},
    {"clipDist", 27},
    {"colorLogic", 26},
    {"cullFace", 25},
    {"debug", 24},
    {"debugSync", 23},
    {"depthClamp", 22},
    {"depthTest", 21},
    {"dither", 20},
    {"framebuffer", 19},
    {"lineSmooth", 18},
    {"multisample", 17},
    {"polygonFill", 16},
    {"polygonLine", 15},
    {"polygonPoint", 14},
    {"polygonSmooth", 13},
    {"primitive", 12},
    {"primitiveIndex", 11},
    {"rasterizerDisgard", 10},
    {"alphaCoverage", 9},
    {"AlphaToOne", 8},
    {"sampleCoverage", 7},
    {"sampleShading", 6},
    {"sampleMask", 5},
    {"scissor", 4},
    {"stencil", 3},
    {"texture", 2},
    {"pointSize", 1},
    
    {"shaders", 4},
    {"lines", 3},
    {"polygons", 2},
    {"texture", 1},

    {"fastest", 3},
    {"nicest", 2},
    {"system", 1},
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