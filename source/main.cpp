#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MSX.h"
#include <3ds.h>

#include <citro2d.h>
extern"C"
{
	#include <SDL/SDL.h>
	#include "fMSX.h"
}

int main()
{
	//osSetSpeedupEnable(1);
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, _NULL);

	hidInit();
	romfsInit();

	srvInit();
	aptInit();
	ptmuInit();
	//aptSetHomeAllowed(true); /* Moved to Init3DS() in 3DSMenu.cpp for detect New3DS/Old3DS */
	//aptSetSleepAllowed(true);
	//APT_SleepIfShellClosed();

	//aptSetSleepAllowed(false);

	//hidInit();

	const char KeysNames[32][16] = {
"A", "B", "SELECT", "START",
"D-pad Right", "D-pad Left", "D-pad Up", "D-pad Down",
"R", "L", "X", "Y", "", "",
"ZL", "ZR", "", "", "", "", "Touch", "", "", "",
"C-stick Right", "C-stick Left", "C-stick Up", "C-stick Down",
"C-pad Right", "C-pad Left", "C-pad Up", "C-pad Down"
	};

	// Main loop
	//while (aptMainLoop())
	//{

		Mode = (Mode & ~MSX_OPTIONS) | MSX_PATCHBDOS;
		fmsxInit();
		//break;
		//if (ExitNow)break;

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();

		//break;
	//}
	//SDL_Quit();
	C2D_Fini();
	C3D_Fini();

	SDL_Quit();

	archiveUnmountAll();
	fsExit();

	//hidExit();

	gfxExit();
	romfsExit();
	ptmuExit();
	aptExit();
	srvExit();
	return 0;
}
