#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psputility.h>
#include "luaplayer.h"

#define MSGDIALOG_UNKNOWN1 0
#define MSGDIALOG_UNKNOWN2 1
#define MSGDIALOG_VISIBLE 2
#define MSGDIALOG_SHUTDOWN_START 3
#define MSGDIALOG_SHUTDOWN_OK 4

static bool msgDialogInitialized = false;

static int lua_msgDialogInitStart(lua_State *L){
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  if (msgDialogInitialized) return 0;

  const char *message = lua_tostring(L, 1);

  SceUtilityMsgDialogParams dialog;

  size_t dialog_size = sizeof(dialog);

  memset(&dialog, 0, dialog_size);

  dialog.size = dialog_size;

  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&dialog.language); // Prompt language
  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,&dialog.buttonSwap); // X/O button swap


  dialog.unknown[0] = 0x11;	// ???
  dialog.unknown[1] = 0x13;
  dialog.unknown[2] = 0x12;
  dialog.unknown[3] = 0x10;
  dialog.unknown[10] = 1;
  strcpy(dialog.message, message);

  sceUtilityMsgDialogInitStart(&dialog);
  msgDialogInitialized = true;

  return 0;
}

static int lua_errDialogInitStart(lua_State *L){
  if (lua_gettop(L) != 1) return luaL_error(L, "wrong number of arguments");
  if (msgDialogInitialized) return 0;

  SceUtilityMsgDialogParams dialog;

  size_t dialog_size = sizeof(dialog);

  memset(&dialog, 0, dialog_size);

  dialog.size = dialog_size;

  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&dialog.language); // Prompt language
  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,&dialog.buttonSwap); // X/O button swap


  dialog.unknown[0] = 0x11;	// ???
  dialog.unknown[1] = 0x13;
  dialog.unknown[2] = 0x12;
  dialog.unknown[3] = 0x10;

  dialog.unknown[10] = 0;
  dialog.unknown[11] = luaL_checkint(L, 1);
  sceUtilityMsgDialogInitStart(&dialog);
  msgDialogInitialized = true;

  return 0;
}

static int lua_dialogGetStatus(lua_State *L){
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");
  if (!msgDialogInitialized) return luaL_error(L, "Dialog not initialized.");
  lua_pushnumber(L,sceUtilityMsgDialogGetStatus());
  return 1;
}

static int lua_dialogUpdate(lua_State *L){
  if (lua_gettop(L) != 0) return luaL_error(L, "no arguments expected.");
  if (!msgDialogInitialized) return luaL_error(L, "Dialog not initialized.");
  
  int ret = sceUtilityMsgDialogGetStatus();
  
  if (ret == 0){
  }else if (ret == 1){
  }else if (ret == 2){
    sceUtilityMsgDialogUpdate(2);
  }else if (ret == 3){
    sceUtilityMsgDialogShutdownStart();
  }else if (ret == 4){
    msgDialogInitialized = false;
  }
  
  lua_pushnumber(L, ret);
  return 1;
}


static const luaL_reg Utility_functions[] = {
  {"msgDialogInit", lua_msgDialogInitStart},
  {"errDialogInit", lua_errDialogInitStart},
  {"dialogStatus", lua_dialogGetStatus},
  {"dialogUpdate", lua_dialogUpdate},
  {0, 0}
};

void luaUtility_init(lua_State *L){
  luaL_openlib(L, "Utility", Utility_functions, 0);
}
