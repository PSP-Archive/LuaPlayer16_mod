#ifndef __LUAPLAYER_H
#define __LUAPLAYER_H

#include <stdlib.h>
#include <tdefs.h>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
extern void luaC_collectgarbage (lua_State *L);
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(val, min, max) ((val)>(max)?(max):((val)<(min)?(min):(val)))

#define UserdataStubs(HANDLE, DATATYPE) \
DATATYPE *to##HANDLE (lua_State *L, int index) \
{ \
  DATATYPE* handle  = (DATATYPE*)lua_touserdata(L, index); \
  if (handle == NULL) luaL_typerror(L, index, #HANDLE); \
  return handle; \
} \
DATATYPE* push##HANDLE(lua_State *L) { \
	DATATYPE * newvalue = (DATATYPE*)lua_newuserdata(L, sizeof(DATATYPE)); \
	luaL_getmetatable(L, #HANDLE); \
	lua_pushvalue(L, -1); \
	lua_setmetatable(L, -3); \
	lua_pushstring(L, "__index"); \
	lua_pushstring(L, #HANDLE); \
	lua_gettable(L, LUA_GLOBALSINDEX); \
	lua_settable(L, -3); \
	lua_pop(L, 1); \
	return newvalue; \
}

#define UserdataRegister(HANDLE, METHODS, METAMETHODS) \
int HANDLE##_register(lua_State *L) { \
	luaL_newmetatable(L, #HANDLE);  /* create new metatable for file handles */ \
	lua_pushliteral(L, "__index"); \
	lua_pushvalue(L, -2);  /* push metatable */ \
	lua_rawset(L, -3);  /* metatable.__index = metatable */ \
	\
	luaL_openlib(L, 0, METAMETHODS, 0); \
	luaL_openlib(L, #HANDLE, METHODS, 0); \
	\
	lua_pushstring(L, #HANDLE); \
	lua_gettable(L, LUA_GLOBALSINDEX); \
	luaL_getmetatable(L, #HANDLE); \
	lua_setmetatable(L, -2); \
	return 1; \
}

#define pushTable(KEY,TKEYL) \
	lua_push##TKEY(L, #KEY); \
	lua_newtable(L);
  
#define pushElement(KEY,VAL,TKEY,TVAL) \
    lua_push##TKEY(L, #KEY); \
    lua_push##TVAL(L, #VAL); \
    lua_settable(L, -3);

extern int runScript(const char* script, bool isStringBuffer);
extern void luaC_collectgarbage (lua_State *L);


extern void luaSound_init(lua_State *L);
extern void luaControls_init(lua_State *L);
extern void luaGraphics_init(lua_State *L);
extern void lua3D_init(lua_State *L);
extern void luaTimer_init(lua_State *L);
extern void luaSystem_init(lua_State *L);
extern void luaWlan_init(lua_State *L);
//extern void luaUtility_init(lua_State *L);

extern void stackDump (lua_State *L);


#endif

