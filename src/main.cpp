/*
 * Lua Player for PSP
 * ------------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE for details.
 *
 * Copyright (c) 2005 Frank Buss <fb@frank-buss.de> (aka Shine)
 *
 * Credits:
 *   many thanks to the authors of the PSPSDK from http://forums.ps2dev.org
 *   and to the hints and discussions from #pspdev on freenode.net
 *
 * $Id: main.cpp 290 2005-12-03 08:49:10Z shine $
 */

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspsdk.h>
#include <psputility.h>
#include <psprtc.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "graphics.h"
#include "sound.h"
#include "luaplayer.h"
// #include "sio.h"

/* the boot.lua */
#include "boot.cpp"

// timezone
static char tz[20];
/* Define the module info section */
PSP_MODULE_INFO("LUAPLAYER", 0x1000, 1, 1);
PSP_MAIN_THREAD_ATTR(0);
PSP_MAIN_THREAD_STACK_SIZE_KB(32); /* smaller stack for kernel thread */

// startup path
char path[256];

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{

	// Unload modules
	unloadMikmod();
		
	sceKernelExitGame();
	return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);

	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}

int nullOutput(const char *buff, int size)
{
	return size;
}

int debugOutput(const char *buff, int size)
{
	static int debugInitialized = 0;
	if(!buff) {
		debugInitialized = 0;
		return 0;
	}
	if (!debugInitialized) {
		disableGraphics();
		pspDebugScreenInit();
		debugInitialized = 1;
	}
	return pspDebugScreenPrintData(buff, size);
}

void initTimezone()
{
	// calculate the difference between UTC and local time
	u64 tick, localTick;
	sceRtcGetCurrentTick(&tick);
	sceRtcConvertUtcToLocalTime(&tick, &localTick);
	int minutesDelta;
	if (tick < localTick) {
		u64 delta = localTick - tick;
		delta /= sceRtcGetTickResolution();
		minutesDelta = delta;
		minutesDelta = -minutesDelta;
	} else {
		u64 delta = tick - localTick;
		delta /= sceRtcGetTickResolution();
		minutesDelta = delta;
	}
	minutesDelta = minutesDelta / 60;

	// calculate the timezone offset
	int tzOffsetAbs = minutesDelta < 0 ? -minutesDelta : minutesDelta;
	int hours = tzOffsetAbs / 60;
	int minutes = tzOffsetAbs - hours * 60;
	sprintf(tz, "GMT%s%02i:%02i", minutesDelta < 0 ? "-" : "+", hours, minutes);
	setenv("TZ", tz, 1);
	tzset();
}

int user_main(SceSize argc, void* argv)
{
	SetupCallbacks();
	initTimezone();

	// init modules
	initGraphics();
	initMikmod();

	// install new output handlers	
	pspDebugInstallStdoutHandler(debugOutput); 
	pspDebugInstallStderrHandler(debugOutput); 

	// execute Lua script (according to boot sequence)
	getcwd(path, 256);
	char* bootStringWith0 = (char*) malloc(size_bootString + 1);
	memcpy(bootStringWith0, bootString, size_bootString);
	bootString[size_bootString] = 0;
	while(1) { // reload on error
		chdir(path); // set base path luaplater/
		clearScreen(0);
		flipScreen();
		clearScreen(0);

		if (runScript(bootStringWith0, true))
		{
			debugOutput("Error: No script file found.\n", 29);
		}
		debugOutput("\nPress start to restart\n", 26);

		SceCtrlData pad; int i;
		sceCtrlReadBufferPositive(&pad, 1); 
		for(i = 0; i < 40; i++) sceDisplayWaitVblankStart();
		while(!(pad.Buttons&PSP_CTRL_START)) sceCtrlReadBufferPositive(&pad, 1); 
		
		debugOutput(0,0);
		initGraphics();
	}
	free(bootStringWith0);
	
	// wait until user ends the program
	sceKernelSleepThread();

	return 0;
}

int main(void)
{
	getcwd(path, 256);
	int err = pspSdkLoadInetModules();
	if (err != 0) {
		pspDebugScreenInit();
		pspDebugScreenPrintf("pspSdkLoadInetModules failed with %x\n", err);
	        sceKernelDelayThread(5*1000000); // 5 sec to read error
	}

	// create user thread, tweek stack size here if necessary
	SceUID thid = sceKernelCreateThread("User Mode Thread", user_main,
	    0x11, // default priority
	    256 * 1024, // stack size (256KB is regular default)
	    PSP_THREAD_ATTR_USER, NULL);
	
	// start user thread, then wait for it to do everything else
	sceKernelStartThread(thid, 0, NULL);
	sceKernelWaitThreadEnd(thid, NULL);
	
	sceKernelExitGame();
	return 0;
}

__attribute__((constructor)) void stdoutInit() 
{ 
	pspKernelSetKernelPC();
	pspSdkInstallNoDeviceCheckPatch();
	pspSdkInstallNoPlainModuleCheckPatch();
	pspKernelSetKernelPC();
	pspKernelSetKernelPC();
	pspSdkInstallNoDeviceCheckPatch();
	pspDebugInstallKprintfHandler(NULL);

	// ignore startup messages from kernel, but install the tty driver in kernel mode
	pspDebugInstallStdoutHandler(nullOutput); 
} 

