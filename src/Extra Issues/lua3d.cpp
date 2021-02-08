#include <malloc.h>
#include <pspdisplay.h>
#include <psputils.h>
#include <pspgu.h>
#include <pspgum.h>
#include "luaplayer.h"

#include "graphics.h"

PspGeContext __attribute__((aligned(16))) geContext;

//ADDED Start
typedef struct{
  unsigned int __attribute__((aligned(16))) contextList[262144];
} GuList;

UserdataStubs(GuList, GuList*)
//ADDED End

extern Color* toColor(lua_State *L, int arg);
extern Image** toImage(lua_State *L, int arg);

static int lua_sceGuClearColor(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuClearColor(*toColor(L, 1));
  return 0;
}

static int lua_sceGuClearDepth(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuClearDepth(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuClear(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuClear(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGumMatrixMode(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGumMatrixMode(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGumLoadIdentity(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments");
  sceGumLoadIdentity();
  return 0;
}
static int lua_sceGumPerspective(lua_State *L) {
  if (lua_gettop(L) != 4) return luaL_error(L, "wrong number of arguments");
  sceGumPerspective(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4));
  return 0;
}

static int lua_sceGumTranslate(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  ScePspFVector3 v;
  v.x = luaL_checknumber(L, 1);
  v.y = luaL_checknumber(L, 2);
  v.z = luaL_checknumber(L, 3);
  sceGumTranslate(&v);
  return 0;
}

static int lua_sceGumRotateXYZ(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  ScePspFVector3 v;
  v.x = luaL_checknumber(L, 1);
  v.y = luaL_checknumber(L, 2);
  v.z = luaL_checknumber(L, 3);
  sceGumRotateXYZ(&v);
  return 0;
}

static int lua_sceGumScale(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  ScePspFVector3 v;
  v.x = luaL_checknumber(L, 1);
  v.y = luaL_checknumber(L, 2);
  v.z = luaL_checknumber(L, 3);
  sceGumScale(&v);
  return 0;
}

// added START
int toCamera(lua_State *L, int index, ScePspFVector3 *cPos, ScePspFVector3 *lPos, ScePspFVector3 *uVec){
  int argc = luaL_getn(L,index);
  if (argc == 3){
    lua_rawgeti(L, index, 1);  //cam pos
      if (lua_type(L,-1)!=LUA_TTABLE) return luaL_error(L, "position table missing");
      if (luaL_getn(L,-1)!=3) return luaL_error(L, "wrong number of components for position");
      lua_rawgeti(L, -1, 1);
        cPos->x = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 2);
        cPos->y = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 3);
        cPos->z = luaL_checknumber(L, -1);
      lua_pop(L, 1);
    lua_pop(L, 1);
    lua_rawgeti(L, index, 2);  //look pos
      if (lua_type(L,-1)!=LUA_TTABLE) return luaL_error(L, "position table missing");
      if (luaL_getn(L,-1)!=3) return luaL_error(L, "wrong number of components for position");
      lua_rawgeti(L, -1, 1);
        lPos->x = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 2);
        lPos->y = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 3);
        lPos->z = luaL_checknumber(L, -1);
      lua_pop(L, 1);
    lua_pop(L, 1);
    lua_rawgeti(L, index, 3);  //up vec
      if (lua_type(L,-1)!=LUA_TTABLE) return luaL_error(L, "position table missing");
      if (luaL_getn(L,-1)!=3) return luaL_error(L, "wrong number of components for position");
      lua_rawgeti(L, -1, 1);
        uVec->x = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 2);
        uVec->y = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 3);
        uVec->z = luaL_checknumber(L, -1);
      lua_pop(L, 1);
    lua_pop(L, 1);
  }else if (argc == 9){
    lua_rawgeti(L, index, 1);
      //cam pos
      lua_rawgeti(L, -1, 1);
        cPos->x = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 2);
        cPos->y = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 3);
        cPos->z = luaL_checknumber(L, -1);
      lua_pop(L, 1);

      //look pos
      lua_rawgeti(L, -1, 1);
        lPos->x = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 2);
        lPos->y = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 3);
        lPos->z = luaL_checknumber(L, -1);
      lua_pop(L, 1);

      //up vec
      lua_rawgeti(L, -1, 1);
        uVec->x = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 2);
        uVec->y = luaL_checknumber(L, -1);
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 3);
        uVec->z = luaL_checknumber(L, -1);
      lua_pop(L, 1);
    lua_pop(L, 1);
  }

  return 0;
}
// added END

static int lua_sceGumLookAt(lua_State *L) {
  int argc = lua_gettop(L);
  if (argc != 1 && argc != 9) return luaL_error(L, "wrong number of arguments");
  ScePspFVector3 cameraPos;
  ScePspFVector3 lookAtPos;
  ScePspFVector3 upVec;
  
  if (argc == 1){
    int ret = toCamera(L, 1, &cameraPos, &lookAtPos, &upVec);
    if (ret != 0) return ret;
  }else{
    cameraPos.x = luaL_checknumber(L, 1);
    cameraPos.y = luaL_checknumber(L, 2);
    cameraPos.z = luaL_checknumber(L, 3);
    lookAtPos.x = luaL_checknumber(L, 4);
    lookAtPos.y = luaL_checknumber(L, 5);
    lookAtPos.z = luaL_checknumber(L, 6);
    upVec.x = luaL_checknumber(L, 7);
    upVec.y = luaL_checknumber(L, 8);
    upVec.z = luaL_checknumber(L, 9);
  }

  sceGumLookAt(&cameraPos, &lookAtPos, &upVec);
  return 0;
}


static int lua_sceGuTexImage(lua_State *L) {
  int argc = lua_gettop(L);
  if (argc != 1 && argc != 2) return luaL_error(L, "wrong number of arguments");
  int mipmap;
  Image* image;
  if (argc == 2){
    mipmap = luaL_checkint(L, 1);
    image = *toImage(L, 2);
  }else{
    mipmap = 0;
    image = *toImage(L, 1);
  }
  sceGuTexImage(mipmap, image->textureWidth, image->textureHeight, image->textureWidth, image->data);
  return 0;
}

static int lua_sceGuTexFunc(lua_State *L) {
  if (lua_gettop(L) != 2) return luaL_error(L, "wrong number of arguments");
  sceGuTexFunc(luaL_checkint(L, 1), luaL_checkint(L, 2));
  return 0;
}

static int lua_sceGuTexEnvColor(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuTexEnvColor(*toColor(L, 1));
  return 0;
}

static int lua_sceGuTexFilter(lua_State *L) {
  if (lua_gettop(L) != 2) return luaL_error(L, "wrong number of arguments");
  sceGuTexFilter(luaL_checkint(L, 1), luaL_checkint(L, 2));
  return 0;
}

static int lua_sceGuTexScale(lua_State *L) {
  if (lua_gettop(L) != 2) return luaL_error(L, "wrong number of arguments");
  sceGuTexScale(luaL_checknumber(L, 1), luaL_checknumber(L, 2));
  return 0;
}

static int lua_sceGuTexOffset(lua_State *L) {
  if (lua_gettop(L) != 2) return luaL_error(L, "wrong number of arguments");
  sceGuTexOffset(luaL_checknumber(L, 1), luaL_checknumber(L, 2));
  return 0;
}

static int lua_sceGuAmbientColor(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuAmbientColor(*toColor(L, 1));
  return 0;
}

static int lua_sceGuAmbient(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuAmbient(*toColor(L, 1));
  return 0;
}

static int lua_sceGuEnable(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuEnable(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuDisable(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuDisable(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuBlendFunc(lua_State *L) {
  if (lua_gettop(L) != 5) return luaL_error(L, "wrong number of arguments");
  sceGuBlendFunc(luaL_checkint(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4), luaL_checkint(L, 5));
  return 0;
}

static int lua_sceGuLight(lua_State *L) {
  if (lua_gettop(L) != 6) return luaL_error(L, "wrong number of arguments");
  ScePspFVector3 v;
  v.x = luaL_checknumber(L, 4);
  v.y = luaL_checknumber(L, 5);
  v.z = luaL_checknumber(L, 6);
  sceGuLight(luaL_checkint(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3), &v);
  return 0;
}

static int lua_sceGuLightAtt(lua_State *L) {
  if (lua_gettop(L) != 4) return luaL_error(L, "wrong number of arguments");
  sceGuLightAtt(luaL_checkint(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4));
  return 0;
}

static int lua_sceGuLightColor(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  sceGuLightColor(luaL_checkint(L, 1), luaL_checkint(L, 2), *toColor(L, 3));
  return 0;
}

static int lua_sceGuLightMode(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuLightMode(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuLightSpot(lua_State *L) {
  if (lua_gettop(L) != 6) return luaL_error(L, "wrong number of arguments");
  ScePspFVector3 v;
  v.x = luaL_checknumber(L, 2);
  v.y = luaL_checknumber(L, 3);
  v.z = luaL_checknumber(L, 4);
  sceGuLightSpot(luaL_checkint(L, 1), &v, luaL_checknumber(L, 5), luaL_checknumber(L, 6));
  return 0;
}

static int lua_sceGumDrawArray(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");

  int prim = luaL_checkint(L, 1);
  int vtype = luaL_checkint(L, 2);
  if (lua_type(L, 3) != LUA_TTABLE) return luaL_error(L, "vertices table missing");
  int n = luaL_getn(L, 3);
  int quads = 0;
  int colorLuaIndex = -1;

  if (vtype & GU_TEXTURE_8BIT || vtype & GU_TEXTURE_16BIT || vtype & GU_TEXTURE_32BITF) quads += 2;
  if (vtype & GU_COLOR_5650 || vtype & GU_COLOR_5551 || vtype & GU_COLOR_4444 || vtype & GU_COLOR_8888) {
    quads++;
    colorLuaIndex = quads;
  }
  if (vtype & GU_NORMAL_8BIT || vtype & GU_NORMAL_16BIT || vtype & GU_NORMAL_32BITF) quads += 3;
  if (vtype & GU_VERTEX_8BIT || vtype & GU_VERTEX_16BIT || vtype & GU_VERTEX_32BITF) quads += 3;

  void* vertices = memalign(16, n * quads*4);
  float* vertex = (float*) vertices;
  int i;
  for (i = 1; i <= n; ++i) {
    // get vertice table
    lua_rawgeti(L, 3, i);
    int n2 = luaL_getn(L, -1);
    if (n2 != quads) {
      free(vertices);
      return luaL_error(L, "wrong number of vertex components");
    }
    int j;
    for (j = 1; j <= n2; ++j) {
      lua_rawgeti(L, -1, j);
      if (j != colorLuaIndex) {
        *vertex = luaL_checknumber(L, -1);
      } else {
        *((Color*) vertex) = *toColor(L, -1);
      }
      lua_pop(L, 1);  // removes 'value'
      vertex++;
    }
    
    // remove vertice table
    lua_pop(L, 1);
  }
  
  sceKernelDcacheWritebackInvalidateAll();
  sceGumDrawArray(prim, vtype, n, NULL, vertices);
  free(vertices);
  return 0;
}

static int lua_start3d(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments");
  sceGeSaveContext(&geContext);
  guStart();
  return 0;
}

static int lua_end3d(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments");
  sceGuFinish();
  sceGuSync(0, 0);
  sceGeRestoreContext(&geContext);
  return 0;
}

//ADDED Start
static int lua_start3dv2(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments");
  guStart();
  return 0;
}

static int lua_end3dv2(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments");
  sceGuFinish();
  sceGuSync(0, 0);
  return 0;
}

static int lua_sceGeSaveContext(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments");
  sceGeSaveContext(&geContext);
  return 0;
}

static int lua_sceGeRestoreContext(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments");
  sceGeRestoreContext(&geContext);
  return 0;
}

static int lua_sceGumDrawBezier(lua_State *L) {
  if (lua_gettop(L) != 4) return luaL_error(L, "wrong number of arguments");

  int vtype = luaL_checkint(L, 1);
  if (lua_type(L, 4) != LUA_TTABLE) return luaL_error(L, "vertices table missing");
  int n = luaL_getn(L, 4);
  int quads = 0;
  int colorLuaIndex = -1;

  if (vtype & GU_TEXTURE_8BIT || vtype & GU_TEXTURE_16BIT || vtype & GU_TEXTURE_32BITF) quads += 2;
  if (vtype & GU_COLOR_5650 || vtype & GU_COLOR_5551 || vtype & GU_COLOR_4444 || vtype & GU_COLOR_8888) {
    quads++;
    colorLuaIndex = quads;
  }
  if (vtype & GU_NORMAL_8BIT || vtype & GU_NORMAL_16BIT || vtype & GU_NORMAL_32BITF) quads += 3;
  if (vtype & GU_VERTEX_8BIT || vtype & GU_VERTEX_16BIT || vtype & GU_VERTEX_32BITF) quads += 3;

  void* vertices = memalign(16, n * quads*4);
  float* vertex = (float*) vertices;
  int i;
  for (i = 1; i <= n; ++i) {
    // get vertice table
    lua_rawgeti(L, 3, i);
    int n2 = luaL_getn(L, -1);
    if (n2 != quads) {
      free(vertices);
      return luaL_error(L, "wrong number of vertex components");
    }
    int j;
    for (j = 1; j <= n2; ++j) {
      lua_rawgeti(L, -1, j);
      if (j != colorLuaIndex) {
        *vertex = luaL_checknumber(L, -1);
      } else {
        *((Color*) vertex) = *toColor(L, -1);
      }
      lua_pop(L, 1);  // removes 'value'
      vertex++;
    }
    
    // remove vertice table
    lua_pop(L, 1);
  }
  
  sceKernelDcacheWritebackInvalidateAll();
  sceGumDrawBezier(vtype,luaL_checkint(L, 2),luaL_checkint(L, 3),NULL,vertices);
  free(vertices);
  return 0;
}

static int lua_sceGumDrawSpline(lua_State *L) {
  if (lua_gettop(L) != 6) return luaL_error(L, "wrong number of arguments");

  int vtype = luaL_checkint(L, 1);
  if (lua_type(L, 6) != LUA_TTABLE) return luaL_error(L, "vertices table missing");
  int n = luaL_getn(L, 6);
  int quads = 0;
  int colorLuaIndex = -1;

  if (vtype & GU_TEXTURE_8BIT || vtype & GU_TEXTURE_16BIT || vtype & GU_TEXTURE_32BITF) quads += 2;
  if (vtype & GU_COLOR_5650 || vtype & GU_COLOR_5551 || vtype & GU_COLOR_4444 || vtype & GU_COLOR_8888) {
    quads++;
    colorLuaIndex = quads;
  }
  if (vtype & GU_NORMAL_8BIT || vtype & GU_NORMAL_16BIT || vtype & GU_NORMAL_32BITF) quads += 3;
  if (vtype & GU_VERTEX_8BIT || vtype & GU_VERTEX_16BIT || vtype & GU_VERTEX_32BITF) quads += 3;

  void* vertices = memalign(16, n * quads*4);
  float* vertex = (float*) vertices;
  int i;
  for (i = 1; i <= n; ++i) {
    // get vertice table
    lua_rawgeti(L, 3, i);
    int n2 = luaL_getn(L, -1);
    if (n2 != quads) {
      free(vertices);
      return luaL_error(L, "wrong number of vertex components");
    }
    int j;
    for (j = 1; j <= n2; ++j) {
      lua_rawgeti(L, -1, j);
      if (j != colorLuaIndex) {
        *vertex = luaL_checknumber(L, -1);
      } else {
        *((Color*) vertex) = *toColor(L, -1);
      }
      lua_pop(L, 1);  // removes 'value'
      vertex++;
    }
    
    // remove vertice table
    lua_pop(L, 1);
  }
  
  sceKernelDcacheWritebackInvalidateAll();
  sceGumDrawSpline(vtype,luaL_checkint(L, 2),luaL_checkint(L, 3),luaL_checkint(L, 4),luaL_checkint(L, 5),NULL,vertices);
  free(vertices);
  return 0;
}

static int lua_sceGuPatchDivide(lua_State *L) {
  if (lua_gettop(L) != 2) return luaL_error(L, "wrong number of arguments");
  sceGuPatchDivide (luaL_checkint(L, 1),luaL_checkint(L, 2));
  return 0;
}

static int lua_sceGuPatchFrontFace(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuPatchFrontFace (luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuPatchPrim(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuPatchPrim (luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuTexSlope(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuTexSlope(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuTexSync(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments");
  sceGuTexSync();
  return 0;
}

static int lua_sceGuCopyImage(lua_State *L) {
  if (lua_gettop(L) != 9) return luaL_error(L, "wrong number of arguments");
  
  Image* src = *toImage(L, 6);

  Image *dest = NULL;
  int type = lua_type(L, 9);
  if (type == LUA_TTABLE) lua_remove(L, 9);
  else if (type == LUA_TUSERDATA) {
    dest = *toImage(L, 9);
    lua_remove(L, 9);
  }

  //Image* dest = *toImage(L, 9);
  if (dest){
    sceGuCopyImage(luaL_checkint(L, 1),luaL_checkint(L, 2),luaL_checkint(L, 3),luaL_checkint(L, 4),luaL_checkint(L, 5),src->textureWidth,src->data,luaL_checkint(L, 7),luaL_checkint(L, 8),dest->textureWidth,dest->data);
  }else{
    Color* vram = getVramDrawBuffer();
    sceKernelDcacheWritebackInvalidateAll();
    sceGuCopyImage(luaL_checkint(L, 1),luaL_checkint(L, 2),luaL_checkint(L, 3),luaL_checkint(L, 4),luaL_checkint(L, 5),src->textureWidth,src->data,luaL_checkint(L, 7),luaL_checkint(L, 8),PSP_LINE_SIZE,vram);
  }
  return 0;
}

static int lua_sceGuAlphaFunc(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  sceGuAlphaFunc(luaL_checkint(L, 1),luaL_checkint(L, 2),luaL_checkint(L, 3));
  return 0;
}

static int lua_sceGuMaterial(lua_State *L) {
  if (lua_gettop(L) != 2) return luaL_error(L, "wrong number of arguments");
  sceGuMaterial (luaL_checkint(L, 1),*toColor(L, 2));
  return 0;
}

static int lua_sceGuModelColor(lua_State *L) {
  if (lua_gettop(L) != 4) return luaL_error(L, "wrong number of arguments");
  sceGuModelColor(*toColor(L, 1),*toColor(L, 2),*toColor(L, 3),*toColor(L, 4));
  return 0;
}

static int lua_sceGuFrontFace(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuFrontFace(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuClearStencil(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuClearStencil(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuPixelMask(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuPixelMask(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuStencilFunc(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  sceGuStencilFunc(luaL_checkint(L, 1),luaL_checkint(L, 2),luaL_checkint(L, 3));
  return 0;
}

static int lua_sceGuStencilOp(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  int x = luaL_checkint(L, 1);
  int y = luaL_checkint(L, 1);
  if ((x < 0 || x > 4095) || (y < 0 || y > 4095)) return luaL_error(L, "offset is between 0 and 4095");
  sceGuStencilOp(luaL_checkint(L, 1),luaL_checkint(L, 2),luaL_checkint(L, 3));
  return 0;
}

static int lua_sceGuLogicalOp(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuLogicalOp(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGuOffset(lua_State *L) {
  if (lua_gettop(L) != 2) return luaL_error(L, "wrong number of arguments");
  sceGuOffset(luaL_checkint(L, 1),luaL_checkint(L, 2));
  return 0;
}

static int lua_sceGuScissor(lua_State *L) {
  if (lua_gettop(L) != 4) return luaL_error(L, "wrong number of arguments");
  sceGuScissor(luaL_checkint(L, 1),luaL_checkint(L, 2),luaL_checkint(L, 3),luaL_checkint(L, 4));
  return 0;
}

static int lua_sceGuViewport(lua_State *L) {
  if (lua_gettop(L) != 4) return luaL_error(L, "wrong number of arguments");
  sceGuViewport(luaL_checkint(L, 1),luaL_checkint(L, 2),luaL_checkint(L, 3),luaL_checkint(L, 4));
  return 0;
}

static int lua_fog(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  sceGuFog(luaL_checkint(L, 1),luaL_checkint(L, 2),*toColor(L, 3));
  return 0;
}

static int lua_texmode(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  sceGuTexMode(luaL_checkint(L, 1),luaL_checkint(L, 2),0,luaL_checkint(L, 3));
  return 0;
}

static int lua_texwrap(lua_State *L) {
  if (lua_gettop(L) != 2) return luaL_error(L, "wrong number of arguments");
  sceGuTexWrap(luaL_checkint(L, 1),luaL_checkint(L, 2));
  return 0;
}

static int lua_texprojmapmode(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuTexProjMapMode(luaL_checkint(L, 1));
  return 0;
}

static int lua_shademodel(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuShadeModel(luaL_checkint(L, 1));
  return 0;
}

static int lua_specular(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuSpecular(luaL_checkint(L, 1));
  return 0;
}

static int lua_color(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuColor(*toColor(L, 1));
  return 0;
}

static int lua_colorfunc(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  sceGuColorFunc(luaL_checkint(L, 1),luaL_checkint(L, 2),luaL_checkint(L, 3));
  return 0;
}

static int lua_colormaterial(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  sceGuColorMaterial(luaL_checkint(L, 1));
  return 0;
}

static int lua_sceGumOrtho(lua_State *L) {
  if (lua_gettop(L) != 6) return luaL_error(L, "wrong number of arguments");
  sceGumOrtho(luaL_checkint(L, 1),luaL_checkint(L, 2),luaL_checkint(L, 3),luaL_checkint(L, 4),luaL_checkint(L, 5),luaL_checkint(L, 6));
  return 0;
}

static int lua_sceGuGetStatus(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  lua_pushboolean(L,sceGuGetStatus(luaL_checkint(L, 1)));
  return 1;
}

static int lua_gumCrossProduct(lua_State *L) {
  if (lua_gettop(L) != 6) return luaL_error(L, "wrong number of arguments");
  ScePspFVector3 r;
  ScePspFVector3 a;
  ScePspFVector3 b;
  a.x = luaL_checkint(L, 1);
  a.y = luaL_checkint(L, 2);
  a.z = luaL_checkint(L, 3);
  b.x = luaL_checkint(L, 4);
  b.y = luaL_checkint(L, 5);
  b.z = luaL_checkint(L, 6);
  gumCrossProduct(&r,&a,&b);
  lua_newtable(L);
    // pushElement(string,number,"x",r.x)
    // pushElement(string,number,"y",r.y)
    // pushElement(string,number,"z",r.z)

    lua_pushstring(L, "x"); lua_pushnumber(L,r.x); lua_settable(L, -3);
    lua_pushstring(L, "y"); lua_pushnumber(L,r.y); lua_settable(L, -3);
    lua_pushstring(L, "z"); lua_pushnumber(L,r.z); lua_settable(L, -3);
  lua_settable(L, -3);
  return 1;
}

static int lua_gumDotProduct(lua_State *L) {
  if (lua_gettop(L) != 6) return luaL_error(L, "wrong number of arguments");
  ScePspFVector3 a;
  ScePspFVector3 b;
  a.x = luaL_checkint(L, 1);
  a.y = luaL_checkint(L, 2);
  a.z = luaL_checkint(L, 3);
  b.x = luaL_checkint(L, 4);
  b.y = luaL_checkint(L, 5);
  b.z = luaL_checkint(L, 6);
  lua_pushnumber(L,gumDotProduct(&a,&b));
  return 1;
}

static int lua_gumNormalize(lua_State *L) {
  if (lua_gettop(L) != 3) return luaL_error(L, "wrong number of arguments");
  ScePspFVector3 v;
  v.x = luaL_checkint(L, 1);
  v.y = luaL_checkint(L, 2);
  v.z = luaL_checkint(L, 3);
  gumNormalize(&v);
  lua_newtable(L);
    // pushElement(string,number,"x",v.x)
    // pushElement(string,number,"y",v.y)
    // pushElement(string,number,"z",v.z)

    lua_pushstring(L, "x"); lua_pushnumber(L,v.x); lua_settable(L, -3);
    lua_pushstring(L, "y"); lua_pushnumber(L,v.y); lua_settable(L, -3);
    lua_pushstring(L, "z"); lua_pushnumber(L,v.z); lua_settable(L, -3);
  lua_settable(L, -3);
  return 1;
}

// Matrix Stuff START
int toMatrix(lua_State *L, int index, ScePspFMatrix4 *m){
  lua_rawgeti(L, index, 1); //x
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component X");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component X");
    lua_rawgeti(L, -1, 1);
      m->x.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m->x.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m->x.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m->x.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, index, 2); //y
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Y");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Y");
    lua_rawgeti(L, -1, 1);
      m->y.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m->y.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m->y.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m->y.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, index, 3); //z
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Z");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Z");
    lua_rawgeti(L, -1, 1);
      m->z.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m->z.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m->z.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m->z.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, index, 4); //w
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component W");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component W");
    lua_rawgeti(L, -1, 1);
      m->w.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m->w.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m->w.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m->w.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  
  return 0;
}

int toMatrix(lua_State *L, int index, ScePspIMatrix4 *m){
  lua_rawgeti(L, index, 1); //x
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component X");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component X");
    lua_rawgeti(L, -1, 1);
      m->x.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m->x.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m->x.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m->x.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, index, 2); //y
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Y");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Y");
    lua_rawgeti(L, -1, 1);
      m->y.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m->y.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m->y.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m->y.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, index, 3); //z
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Z");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Z");
    lua_rawgeti(L, -1, 1);
      m->z.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m->z.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m->z.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m->z.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, index, 4); //w
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component W");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component W");
    lua_rawgeti(L, -1, 1);
      m->w.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m->w.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m->w.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m->w.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  
  return 0;
}

static int lua_sceGumMultMatrix(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  if (lua_type(L,1) != LUA_TTABLE) return luaL_error(L, "matrix table missing");
  if (luaL_getn(L,1)!=4) return luaL_error(L, "wrong number of matrix components");

  ScePspFMatrix4 m;

  // If this DOES NOT work, comment these two lines out and take out the "/* */" on the code below.
  // If this DOES work, you can delete the code that is commeted with "/* */" below
  int ret = toMatrix(L,1,&m);
  if (ret != 0) return ret;

/*
  lua_rawgeti(L, 1, 1); //x
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component X");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component X");
    lua_rawgeti(L, -1, 1);
      m.x.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.x.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.x.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.x.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 1, 2); //y
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Y");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Y");
    lua_rawgeti(L, -1, 1);
      m.y.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.y.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.y.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.y.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 1, 3); //z
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Z");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Z");
    lua_rawgeti(L, -1, 1);
      m.z.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.z.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.z.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.z.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 1, 4); //w
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component W");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component W");
    lua_rawgeti(L, -1, 1);
      m.w.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.w.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.w.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.w.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
*/

  sceGumMultMatrix(&m);
  return 0;
}

static int lua_sceGuSetMatrix(lua_State *L) {
  if (lua_gettop(L) != 2) return luaL_error(L, "wrong number of arguments");
  if (lua_type(L,2) != LUA_TTABLE) return luaL_error(L, "matrix table missing");
  if (luaL_getn(L,2)!=4) return luaL_error(L, "wrong number of matrix components");

  ScePspFMatrix4 m;

  // If this DOES NOT work, comment these two lines out and take out the "/* */" on the code below.
  // If this DOES work, you can delete the code that is commeted with "/* */" below
  int ret = toMatrix(L,2,&m);
  if (ret != 0) return ret;

/*
  lua_rawgeti(L, 2, 1); //x
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component X");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component X");
    lua_rawgeti(L, -1, 1);
      m.x.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.x.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.x.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.x.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 2, 2); //y
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Y");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Y");
    lua_rawgeti(L, -1, 1);
      m.y.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.y.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.y.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.y.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 2, 3); //z
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Z");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Z");
    lua_rawgeti(L, -1, 1);
      m.z.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.z.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.z.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.z.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 2, 4); //w
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component W");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component W");
    lua_rawgeti(L, -1, 1);
      m.w.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.w.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.w.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.w.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
*/

  sceGuSetMatrix(luaL_checknumber(L,1),&m);
  return 0;
}

static int lua_sceGuSetDither(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  if (lua_type(L,1) != LUA_TTABLE) return luaL_error(L, "matrix table missing");
  if (luaL_getn(L,1)!=4) return luaL_error(L, "wrong number of matrix components");

  ScePspIMatrix4 m;

  // If this DOES NOT work, comment these two lines out and take out the "/* */" on the code below.
  // If this DOES work, you can delete the code that is commeted with "/* */" below
  int ret = toMatrix(L,1,&m);
  if (ret != 0) return ret;

/*
  lua_rawgeti(L, 1, 1); //x
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component X");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component X");
    lua_rawgeti(L, -1, 1);
      m.x.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.x.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.x.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.x.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 1, 2); //y
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Y");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Y");
    lua_rawgeti(L, -1, 1);
      m.y.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.y.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.y.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.y.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 1, 3); //z
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Z");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Z");
    lua_rawgeti(L, -1, 1);
      m.z.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.z.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.z.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.z.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 1, 4); //w
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component W");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component W");
    lua_rawgeti(L, -1, 1);
      m.w.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.w.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.w.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.w.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
*/

  sceGuSetDither(&m);
  return 0;
}

static int lua_sceGumLoadMatrix(lua_State *L) {
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  if (lua_type(L,1) != LUA_TTABLE) return luaL_error(L, "matrix table missing");
  if (luaL_getn(L,1)!=4) return luaL_error(L, "wrong number of matrix components");

  ScePspFMatrix4 m;
  
  // If this DOES NOT work, comment these two lines out and take out the "/* */" on the code below.
  // If this DOES work, you can delete the code that is commeted with "/* */" below
  int ret = toMatrix(L,1,&m);
  if (ret != 0) return ret;

/*
  lua_rawgeti(L, 1, 1); //x
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component X");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component X");
    lua_rawgeti(L, -1, 1);
      m.x.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.x.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.x.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.x.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 1, 2); //y
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Y");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Y");
    lua_rawgeti(L, -1, 1);
      m.y.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.y.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.y.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.y.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 1, 3); //z
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component Z");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component Z");
    lua_rawgeti(L, -1, 1);
      m.z.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.z.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.z.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.z.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
  lua_rawgeti(L, 1, 4); //w
    if (lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "matrix table missing for component W");
    if (luaL_getn(L,-1)!=4) return luaL_error(L, "wrong number of matrix components for component W");
    lua_rawgeti(L, -1, 1);
      m.w.x = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
      m.w.y = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
      m.w.z = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
      m.w.w = luaL_checknumber(L, -1);
    lua_pop(L, 1);
  lua_pop(L, 1);
*/

  sceGumLoadMatrix(&m);
  return 0;
}

static int lua_sceGumStoreMatrix(lua_State *L){
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");

  ScePspFMatrix4 m;

  sceGumStoreMatrix(&m);  // save current matrix to m

  lua_newtable(L);
    lua_pushstring(L, "x"); lua_newtable(L);
      lua_pushstring(L, "x"); lua_pushnumber(L,m.x.x); lua_settable(L, -3);
      lua_pushstring(L, "y"); lua_pushnumber(L,m.x.y); lua_settable(L, -3);
      lua_pushstring(L, "z"); lua_pushnumber(L,m.x.z); lua_settable(L, -3);
      lua_pushstring(L, "w"); lua_pushnumber(L,m.x.w); lua_settable(L, -3);
    lua_settable(L, -3); 
    lua_pushstring(L, "y"); lua_newtable(L);
      lua_pushstring(L, "x"); lua_pushnumber(L,m.y.x); lua_settable(L, -3);
      lua_pushstring(L, "y"); lua_pushnumber(L,m.y.y); lua_settable(L, -3);
      lua_pushstring(L, "z"); lua_pushnumber(L,m.y.z); lua_settable(L, -3);
      lua_pushstring(L, "w"); lua_pushnumber(L,m.y.w); lua_settable(L, -3);
    lua_settable(L, -3);
    lua_pushstring(L, "z"); lua_newtable(L);
      lua_pushstring(L, "x"); lua_pushnumber(L,m.z.x); lua_settable(L, -3);
      lua_pushstring(L, "y"); lua_pushnumber(L,m.z.y); lua_settable(L, -3);
      lua_pushstring(L, "z"); lua_pushnumber(L,m.z.z); lua_settable(L, -3);
      lua_pushstring(L, "w"); lua_pushnumber(L,m.z.w); lua_settable(L, -3);
    lua_settable(L, -3);
    lua_pushstring(L, "w"); lua_newtable(L);
      lua_pushstring(L, "x"); lua_pushnumber(L,m.w.x); lua_settable(L, -3);
      lua_pushstring(L, "y"); lua_pushnumber(L,m.w.y); lua_settable(L, -3);
      lua_pushstring(L, "z"); lua_pushnumber(L,m.w.z); lua_settable(L, -3);
      lua_pushstring(L, "w"); lua_pushnumber(L,m.w.w); lua_settable(L, -3);
    lua_settable(L, -3);
  lua_settable(L, -3);

  return 1;
}

static int lua_sceGumPopMatrix(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");
  sceGumPopMatrix();
  return 0;
}

static int lua_sceGumPushMatrix(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");
  sceGumPushMatrix();
  return 0;
}

static int lua_sceGumUpdateMatrix(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");
  sceGumUpdateMatrix();
  return 0;
}

static int lua_sceGumFullInverse(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");
  sceGumFullInverse();
  return 0;
}

static int lua_sceGumFastInverse(lua_State *L) {
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");
  sceGumFastInverse();
  return 0;
}
// Matrix Stuff END

// GuList Stuff
/*
  //Functions
static int GuList_newList(lua_State *L){
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");
  GuList** luaGuList = pushGuList(L);
  GuList* guList = (GuList*) malloc(sizeof(GuList));
  *luaGuList = guList;
  return 1;
}

static int GuList_size(lua_State *L){
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");
  lua_pushnumber(L,sceGuCheckList());
  return 1;
}

static int GuList_finish(lua_State *L){
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");
  lua_pushnumber(L,sceGuCheckList());
  sceGuFinish();
  sceGuSync(0,0);
  return 1;
}

  //Methods
static int GuList_start(lua_State *L){
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  GuList* guList = *toGuList(L,1);
  sceGuStart(GU_CALL,guList->contextList);
  return 0;
}

static int GuList_call(lua_State *L){
  if (lua_gettop(L) != 1) return luaL_error(L, "no arguments expected.");
  GuList* guList = *toGuList(L,1);
  sceGuCallList(guList->contextList);
  return 0;
}

  //Meta
static int GuList_free(lua_State *L){
  GuList* guList = *toGuList(L, 1);
  free(guList);
  return 0;
}

  //Register
static const luaL_reg GuList_methods[] = {  //class functions.  Meaning they use ":" instead of ".".
  {"start",GuList_start},
  {"call",GuList_call},
  {0,0}
};


static const luaL_reg GuList_meta[] = {
  {"__gc", GuList_free},
  {0,0}
};

UserdataRegister(GuList, GuList_methods, GuList_meta)

static const luaL_reg GuList_functions[] = {
  {"new", GuList_newList},
  {"finish",GuList_finish},
  {"size",GuList_size},
  {0,0}
};
//ADDED End
*/

static const luaL_reg Gu_functions[] = {
	{"clearColor", lua_sceGuClearColor},
	{"clearDepth", lua_sceGuClearDepth},
	{"clear", lua_sceGuClear},
	{"texImage", lua_sceGuTexImage},
	{"texFunc", lua_sceGuTexFunc},
	{"texEnvColor", lua_sceGuTexEnvColor},
	{"texFilter", lua_sceGuTexFilter},
	{"texScale", lua_sceGuTexScale},
	{"texOffset", lua_sceGuTexOffset},
	{"ambientColor", lua_sceGuAmbientColor},
	{"ambient", lua_sceGuAmbient},
	{"enable", lua_sceGuEnable},
	{"disable", lua_sceGuDisable},
	{"blendFunc", lua_sceGuBlendFunc},
	{"light", lua_sceGuLight},
	{"lightAtt", lua_sceGuLightAtt},
	{"lightColor", lua_sceGuLightColor},
	{"lightMode", lua_sceGuLightMode},
	{"lightSpot", lua_sceGuLightSpot},
	{"start3d", lua_start3d},
	{"end3d", lua_end3d},
	{"fog", lua_fog},
	{"texMode", lua_texmode},
	{"texWrap", lua_texwrap},
	{"texProjMapMode", lua_texprojmapmode},
	{"shadeModel", lua_shademodel},
	{"specular", lua_specular},
	{"color", lua_color},
	{"colorFunc", lua_colorfunc},
	{"colorMaterial", lua_colormaterial},
	{"patchDivide",lua_sceGuPatchDivide},
	{"patchFrontFace",lua_sceGuPatchFrontFace},
	{"patchPrim",lua_sceGuPatchPrim},
	{"texSlope",lua_sceGuTexSlope},
        {"texSync",lua_sceGuTexSync},
        {"copyImage",lua_sceGuCopyImage},
        {"alphaFunc",lua_sceGuAlphaFunc},
        {"material",lua_sceGuMaterial},
        {"modelColor",lua_sceGuModelColor},
        {"frontFace",lua_sceGuFrontFace},
        {"clearStencil",lua_sceGuClearStencil},
        {"pixelMask",lua_sceGuPixelMask},
        {"stencilFunc",lua_sceGuStencilFunc},
        {"stencilOp",lua_sceGuStencilOp},
        {"logicalOp",lua_sceGuLogicalOp},
        {"offset",lua_sceGuOffset},
        {"scissor",lua_sceGuScissor},
        {"viewport",lua_sceGuViewport},
        {"getStatus",lua_sceGuGetStatus},
        {"start3dv2",lua_start3dv2},
        {"end3dv2",lua_end3dv2},
        {"geSaveContext",lua_sceGeSaveContext},
        {"geRestoreContext",lua_sceGeRestoreContext},
        // Matrix
        {"setMatrix",lua_sceGuSetMatrix},
        {"setDither",lua_sceGuSetDither},
	{0, 0}
};

static const luaL_reg Gum_functions[] = {
	{"matrixMode", lua_sceGumMatrixMode},
	{"loadIdentity", lua_sceGumLoadIdentity},
	{"perspective", lua_sceGumPerspective},
	{"translate", lua_sceGumTranslate},
	{"scale", lua_sceGumScale},
	{"rotateXYZ", lua_sceGumRotateXYZ},
	{"drawArray", lua_sceGumDrawArray},
	{"drawBezier", lua_sceGumDrawBezier},
	{"drawSpline", lua_sceGumDrawSpline},
	{"lookAt",     lua_sceGumLookAt},
        {"ortho",lua_sceGumOrtho},
        {"crossProduct",lua_gumCrossProduct},
        {"dotProduct",lua_gumDotProduct},
        {"normalize",lua_gumNormalize},
        // Matrix
        {"multMatrix",lua_sceGumMultMatrix},
        {"loadMatrix",lua_sceGumLoadMatrix},
        {"storeMatrix",lua_sceGumStoreMatrix},
        {"popMatrix",lua_sceGumPopMatrix},
        {"pushMatrix",lua_sceGumPushMatrix},
        {"updateMatrix",lua_sceGumUpdateMatrix},
        {"fullInverse",lua_sceGumFullInverse},
        {"fastInverse",lua_sceGumFastInverse},
	{0, 0}
};

void lua3D_init(lua_State *L) {
  luaL_openlib(L, "Gu", Gu_functions, 0);
  //GuList_register(L);
  //luaL_openlib(L, "GuList", GuList_functions, 0);
  luaL_openlib(L, "Gum", Gum_functions, 0);

#define GU_CONSTANT(name)\
  lua_pushstring(L, #name);\
  lua_pushnumber(L, GU_##name);\
  lua_settable(L, -3);

  lua_pushstring(L, "Gu");
  lua_gettable(L, LUA_GLOBALSINDEX);
  GU_CONSTANT(PI)
  GU_CONSTANT(FALSE)
  GU_CONSTANT(TRUE)
  GU_CONSTANT(POINTS)
  GU_CONSTANT(LINES)
  GU_CONSTANT(LINE_STRIP)
  GU_CONSTANT(TRIANGLES)
  GU_CONSTANT(TRIANGLE_STRIP)
  GU_CONSTANT(TRIANGLE_FAN)
  GU_CONSTANT(SPRITES)
  GU_CONSTANT(ALPHA_TEST)
  GU_CONSTANT(DEPTH_TEST)
  GU_CONSTANT(SCISSOR_TEST)
  GU_CONSTANT(STENCIL_TEST)
  GU_CONSTANT(BLEND)
  GU_CONSTANT(CULL_FACE)
  GU_CONSTANT(DITHER)
  GU_CONSTANT(FOG)
  GU_CONSTANT(CLIP_PLANES)
  GU_CONSTANT(TEXTURE_2D)
  GU_CONSTANT(LIGHTING)
  GU_CONSTANT(LIGHT0)
  GU_CONSTANT(LIGHT1)
  GU_CONSTANT(LIGHT2)
  GU_CONSTANT(LIGHT3)
  GU_CONSTANT(LINE_SMOOTH)
  GU_CONSTANT(PATCH_CULL_FACE)
  GU_CONSTANT(COLOR_TEST)
  GU_CONSTANT(COLOR_LOGIC_OP)
  GU_CONSTANT(FACE_NORMAL_REVERSE)
  GU_CONSTANT(PATCH_FACE)
  GU_CONSTANT(FRAGMENT_2X)
  GU_CONSTANT(PROJECTION)
  GU_CONSTANT(VIEW)
  GU_CONSTANT(MODEL)
  GU_CONSTANT(TEXTURE)
  GU_CONSTANT(TEXTURE_8BIT)
  GU_CONSTANT(TEXTURE_16BIT)
  GU_CONSTANT(TEXTURE_32BITF)
  GU_CONSTANT(TEXTURE_BITS)
  /*Color RES_1-3 removed from sdk
  GU_CONSTANT(COLOR_RES1)
  GU_CONSTANT(COLOR_RES2)
  GU_CONSTANT(COLOR_RES3) */
  GU_CONSTANT(COLOR_5650)
  GU_CONSTANT(COLOR_5551)
  GU_CONSTANT(COLOR_4444)
  GU_CONSTANT(COLOR_8888)
  GU_CONSTANT(COLOR_BITS)
  GU_CONSTANT(NORMAL_8BIT)
  GU_CONSTANT(NORMAL_16BIT)
  GU_CONSTANT(NORMAL_32BITF)
  GU_CONSTANT(NORMAL_BITS)
  GU_CONSTANT(VERTEX_8BIT)
  GU_CONSTANT(VERTEX_16BIT)
  GU_CONSTANT(VERTEX_32BITF)
  GU_CONSTANT(VERTEX_BITS)
  GU_CONSTANT(WEIGHT_8BIT)
  GU_CONSTANT(WEIGHT_16BIT)
  GU_CONSTANT(WEIGHT_32BITF)
  GU_CONSTANT(WEIGHT_BITS)
  GU_CONSTANT(INDEX_8BIT)
  GU_CONSTANT(INDEX_16BIT)
  GU_CONSTANT(INDEX_BITS)
  GU_CONSTANT(WEIGHTS_BITS)
  GU_CONSTANT(VERTICES_BITS)
  GU_CONSTANT(TRANSFORM_3D)
  GU_CONSTANT(TRANSFORM_2D)
  GU_CONSTANT(TRANSFORM_BITS)
  GU_CONSTANT(PSM_5650)
  GU_CONSTANT(PSM_5551)
  GU_CONSTANT(PSM_4444)
  GU_CONSTANT(PSM_8888)
  GU_CONSTANT(PSM_T4)
  GU_CONSTANT(PSM_T8)
  GU_CONSTANT(PSM_T16)
  GU_CONSTANT(PSM_T32)
  GU_CONSTANT(FLAT)
  GU_CONSTANT(SMOOTH)
  GU_CONSTANT(CLEAR)
  GU_CONSTANT(AND)
  GU_CONSTANT(AND_REVERSE)
  GU_CONSTANT(COPY)
  GU_CONSTANT(AND_INVERTED)
  GU_CONSTANT(NOOP)
  GU_CONSTANT(XOR)
  GU_CONSTANT(OR)
  GU_CONSTANT(NOR)
  GU_CONSTANT(EQUIV)
  GU_CONSTANT(INVERTED)
  GU_CONSTANT(OR_REVERSE)
  GU_CONSTANT(COPY_INVERTED)
  GU_CONSTANT(OR_INVERTED)
  GU_CONSTANT(NAND)
  GU_CONSTANT(SET)
  GU_CONSTANT(NEAREST)
  GU_CONSTANT(LINEAR)
  GU_CONSTANT(NEAREST_MIPMAP_NEAREST)
  GU_CONSTANT(LINEAR_MIPMAP_NEAREST)
  GU_CONSTANT(NEAREST_MIPMAP_LINEAR)
  GU_CONSTANT(LINEAR_MIPMAP_LINEAR)
  GU_CONSTANT(TEXTURE_COORDS)
  GU_CONSTANT(TEXTURE_MATRIX)
  GU_CONSTANT(ENVIRONMENT_MAP)
  GU_CONSTANT(POSITION)
  GU_CONSTANT(UV)
  GU_CONSTANT(NORMALIZED_NORMAL)
  GU_CONSTANT(NORMAL)
  GU_CONSTANT(REPEAT)
  GU_CONSTANT(CLAMP)
  GU_CONSTANT(CW)
  GU_CONSTANT(CCW)
  GU_CONSTANT(NEVER)
  GU_CONSTANT(ALWAYS)
  GU_CONSTANT(EQUAL)
  GU_CONSTANT(NOTEQUAL)
  GU_CONSTANT(LESS)
  GU_CONSTANT(LEQUAL)
  GU_CONSTANT(GREATER)
  GU_CONSTANT(GEQUAL)
  GU_CONSTANT(COLOR_BUFFER_BIT)
  GU_CONSTANT(STENCIL_BUFFER_BIT)
  GU_CONSTANT(DEPTH_BUFFER_BIT)
  GU_CONSTANT(TFX_MODULATE)
  GU_CONSTANT(TFX_DECAL)
  GU_CONSTANT(TFX_BLEND)
  GU_CONSTANT(TFX_REPLACE)
  GU_CONSTANT(TFX_ADD)
  GU_CONSTANT(TCC_RGB)
  GU_CONSTANT(TCC_RGBA)
  GU_CONSTANT(ADD)
  GU_CONSTANT(SUBTRACT)
  GU_CONSTANT(REVERSE_SUBTRACT)
  GU_CONSTANT(MIN)
  GU_CONSTANT(MAX)
  GU_CONSTANT(ABS)
  GU_CONSTANT(SRC_COLOR)
  GU_CONSTANT(ONE_MINUS_SRC_COLOR)
  GU_CONSTANT(SRC_ALPHA)
  GU_CONSTANT(ONE_MINUS_SRC_ALPHA)
  GU_CONSTANT(DST_COLOR)
  GU_CONSTANT(ONE_MINUS_DST_COLOR)
  GU_CONSTANT(DST_ALPHA)
  GU_CONSTANT(ONE_MINUS_DST_ALPHA)
  GU_CONSTANT(FIX)
  GU_CONSTANT(KEEP)
  GU_CONSTANT(ZERO)
  GU_CONSTANT(REPLACE)
  GU_CONSTANT(INVERT)
  GU_CONSTANT(INCR)
  GU_CONSTANT(DECR)
  GU_CONSTANT(AMBIENT)
  GU_CONSTANT(DIFFUSE)
  GU_CONSTANT(SPECULAR)
  GU_CONSTANT(AMBIENT_AND_DIFFUSE)
  GU_CONSTANT(DIFFUSE_AND_SPECULAR)
  GU_CONSTANT(UNKNOWN_LIGHT_COMPONENT)
  GU_CONSTANT(DIRECTIONAL)
  GU_CONSTANT(POINTLIGHT)
  GU_CONSTANT(SPOTLIGHT)
  GU_CONSTANT(DIRECT)
  GU_CONSTANT(CALL)
  GU_CONSTANT(SEND)
  GU_CONSTANT(TAIL)
  GU_CONSTANT(HEAD)
}
