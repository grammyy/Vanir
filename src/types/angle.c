#include "common.h"

static int toStringAngle(lua_State *L) {
    float roll = getfieldf(L, 1, "roll");
    float pitch = getfieldf(L, 1, "pitch");
    float yaw  = getfieldf(L, 1, "yaw");

    lua_pushfstring(L, "(%f, %f, %f)", roll, pitch, yaw);

    return 1;
}

static const luaL_Reg angleMeta[] = {
    { "__tostring", toStringAngle },

    { NULL, NULL }
};

int Angle(lua_State *L) {
    float roll = (float)luaL_optnumber(L, 1, 0.0);
    float pitch = (float)luaL_optnumber(L, 2, 0.0);
    float yaw  = (float)luaL_optnumber(L, 3, 0.0);

    lua_newtable(L);
    setFieldNumber(L, "roll",  roll);
    setFieldNumber(L, "pitch", pitch);
    setFieldNumber(L, "yaw",   yaw);

    addMethods(L, "vanir.Angle", NULL, angleMeta);

    return 1;
}