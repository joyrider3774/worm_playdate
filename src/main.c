#include <stdio.h>
#include <stdlib.h>
#include "pd_api.h"
#include "worm.h"

#ifdef _WINDLL
__declspec(dllexport)
#endif

int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg)
{

	if ( event == kEventInit )
	{
		setPDPtr(playdate);
		playdate->system->logToConsole("app.uuid=9eb248b6b44f4d6481fe3058080cbf7a");
		playdate->display->setRefreshRate(FPS);
		//original graphcis were 8x8 for a screen resolution of 128x64 (arduboy) i scaled the graphics by 3
		//so we have some space left and can use a drawoffset to center everything on the screen
		playdate->graphics->setDrawOffset((400-ScreenWidth) >> 1, (240 - ScreenHeight) >> 1);
		playdate->system->setUpdateCallback(mainLoop, NULL);
		setupGame();
	}

	if (event == kEventTerminate)
	{
		terminateGame();
	}
	
	return 0;
}
