
// Includes
//{{{

#define GLOBALS_C
#include "globals.h"

//}}}


// Functions
//{{{

void globals_init()
{

	g_viewangletangent = 1*UPM;
	// FLAG put the below code into a resize function, call that function here
	{ //pseudoresize
		int w, h;
		w = 600;
		h = 400;
		// Check wh ratio against max allowed, adjust as necessary, find
		//  out how to draw black bars on side of window FLAG
		g_windowwidth=w; // in pixels, obtain from SDL
		g_windowheight=h;
		g_windowsquare = (w>h)?h:w;
	}

}

//}}}

