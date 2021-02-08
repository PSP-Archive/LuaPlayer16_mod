//
//  utility.c
//  Miscellaneous useful things for Ruckus
//
//  Licensed under the BSD license. See LICENSE for details.
//  Copyright (c) 2006 Daniel S. Neumeyer <dan@captaindan.org>
//

#include "utility.h"
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspdebug.h>
#include <pspkernel.h>
#include <malloc.h>


void rksUtilityOhShit(const char *message, const char *file, int line)
{
	sceGuTerm();
	pspDebugScreenInit();
	pspDebugScreenPrintf("Oops!\n\n");
	pspDebugScreenPrintf("%s\n\n", message);
	pspDebugScreenPrintf("I'm afraid I can't go on. Sorry about that.\n");
	pspDebugScreenPrintf("Press your \"Home\" button to get out of this.\n\n");
	pspDebugScreenSetXY(0, 33);
	pspDebugScreenPrintf("%s:%d %s", file, line, RUCKUS_VERSION_STRING);
	while (1) { sceDisplayWaitVblankStart(); }
}


int rksUtilityExitCallback(int a, int b, void *c)
{
	sceKernelExitGame();
	return 0;
}


int rksUtilityCallbackThread(SceSize args, void *argp)
{
	sceKernelRegisterExitCallback(sceKernelCreateCallback("Exit Callback", rksUtilityExitCallback, NULL));
	sceKernelSleepThreadCB();
	return 0;
}


int rksUtilitySetupCallbacks()
{
	int threadID = sceKernelCreateThread("My Thread", rksUtilityCallbackThread, 0x11, 0x0FA0, 0, 0);
	if (threadID >= 0) sceKernelStartThread(threadID, 0, 0);
	return threadID;
}
