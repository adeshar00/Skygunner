
// Includes
//{{{
#ifndef EMSCRIPTEN
	// Native
	#include <SDL2/SDL.h>
#else
	// Web
	#include <SDL.h>
	#include <emscripten.h>
#endif

#include <stdlib.h> // FLAG to make compiler shut up about exit

#define INPUT_C
#include "globals.h"
#include "input.h"
#include "ui.h"
#include "intmath.h"
#include "thread.h"
#include "debug.h"
#include "render.h"
//}}}


// Internal Functions
//{{{

/*
   Note: Internal functions do no lock input mutex, make sure appropriate locking is
    in place when calling these.
   */

void input_reset()
{
	/*
	   Resets all input variables that represent "one off" data, such as
	    key presses (vs keyholds which are set to false when a key is released),
	    mouse presses, and mousewheel scrolling.
	   Should only be called once per tick, so no keypresses are lost
	   */

	// Reset keypresses
	{
		int i;
		for(i=0;i<INPUT_KEYS;i++)
		{
			in_keypress[i] = 0;
		}
	}
	in_leftmousepress = 0;
	in_rightmousepress = 0;
	in_deltamousewheel = 0;

}

void input_update()
{
	/*
	   Converts SDL event data into internal format which serves as an intermediary
	    between SDL events and input relevent to specific game phases.
	   Unlike input_reset, can be called an arbitrary number of times per tick, from
	    either thread.
	   */

	// Update key vars
	while(SDL_PollEvent(&in_event))
	{
		switch(in_event.type)
		{
			case SDL_MOUSEMOTION:
				in_cursorx = in_event.motion.x;
				in_cursory = in_event.motion.y;
			break;

			case SDL_MOUSEBUTTONDOWN:
				switch(in_event.button.button)
				{
					case SDL_BUTTON_LEFT:
						in_leftmousepress = 1;
						in_leftmousehold = 1;
					break;

					case SDL_BUTTON_RIGHT:
						in_rightmousepress = 1;
						in_rightmousehold = 1;
					break;
				}	
			break;

			case SDL_MOUSEBUTTONUP:
				switch(in_event.button.button)
				{
					case SDL_BUTTON_LEFT:
						in_leftmousehold = 0;
					break;

					case SDL_BUTTON_RIGHT:
						in_rightmousehold = 0;
					break;
				}	
			break;

			case SDL_MOUSEWHEEL:
				in_deltamousewheel+= in_event.wheel.y;
				break;


			case SDL_KEYDOWN: // FLAG 'a' is 97
				if(in_event.key.keysym.sym<INPUT_KEYS)
				{
					in_keypress[in_event.key.keysym.sym] = 1;
					in_keyhold[in_event.key.keysym.sym] = 1;
					if(in_event.key.keysym.sym==113) exit(1); // FLAG instaclose
					//printf("%d\n",in_event.key.keysym.sym); // FLAG
				}
			break;

			case SDL_KEYUP:
				if(in_event.key.keysym.sym<INPUT_KEYS)
				{
					in_keyhold[in_event.key.keysym.sym] = 0;

				}
			break;

			case SDL_WINDOWEVENT:
			switch(in_event.window.event)
			{
				// for full list see http://wiki.libsdl.org/SDL_WindowEvent

				case SDL_WINDOWEVENT_CLOSE:
				exit(1); // FLAG make a function for forced closure
				break;

				case SDL_WINDOWEVENT_RESIZED:
				{
					render_resize(in_event.window.data1,in_event.window.data2);
					debug_message("Resize: %d %d\n",
							in_event.window.data1,
							in_event.window.data2);
					//xx
				}
				break;
			}
			break;
		}
	}

}
//}}}

// External Functions
//{{{

void input_init()
{
	/*
	   Initializes global input variables, should be called at program initialization
	   */
	int i;

	thread_lockinputmutex();

	for(i=0;i<INPUT_KEYS;i++)
	{
		in_keyhold[i] = 0;
		in_keypress[i] = 0;
	}
	in_leftmousepress = 0;
	in_leftmousehold = 0;
	in_rightmousepress = 0;
	in_rightmousehold = 0;
	in_deltamousewheel = 0;

	thread_unlockinputmutex();
}

void input_menu()
{
	thread_lockinputmutex();

	input_reset();
	input_update();

	thread_unlockinputmutex();
}

void input_fetchmousecoords(intv* x, intv* y)
{
	*x = (in_cursorx-g_windowwidth/2)*UPM/(g_windowsquare/2);
	*y = -(in_cursory-g_windowheight/2)*UPM/(g_windowsquare/2);
}

inputdatabattle input_battle() // Runs during battle gamephase
{
	// Temp Variables
	inputdatabattle d;
	static int lastcx;
	static int lastcy;

	// Lock Mutex
	thread_lockinputmutex();

	// Update Input Variables
	input_update();


	// Booleans
	d.forward = in_keyhold[SDLK_w];
	d.back = in_keyhold[SDLK_s];
	d.left = in_keyhold[SDLK_a];
	d.right = in_keyhold[SDLK_d];
	d.up = in_keyhold[SDLK_SPACE];
	d.down = in_keyhold[SDLK_c];
	d.triggerpress = in_leftmousepress;
	d.triggerhold = in_leftmousehold;

	// Weapon Change
	d.gunchange = -1;
	if(in_keypress[SDLK_5]) d.gunchange = 4;
	if(in_keypress[SDLK_4]) d.gunchange = 3;
	if(in_keypress[SDLK_3]) d.gunchange = 2;
	if(in_keypress[SDLK_2]) d.gunchange = 1;
	if(in_keypress[SDLK_1]) d.gunchange = 0;

	// Ammo Change
	d.ammochange = in_deltamousewheel;

	// Cam/chasis Rotate
	//if(in_keyhold[SDLK_f])
	if(in_rightmousehold)
	{
		// dx/y : percentage of a half windowsquare that the mouse has moved
		intv dx = (in_cursorx-lastcx)*UPM/(g_windowsquare/2);
		intv dy = (in_cursory-lastcy)*UPM/(g_windowsquare/2);
		d.rotatex=VMV(VMV(2*UMV(dx,g_viewangletangent),-PI2),ROTATEMOD);
		d.rotatey=VMV(VMV(2*UMV(dy,g_viewangletangent),-PI2),ROTATEMOD);
	}
	else
	{
		d.rotatex = 0;
		d.rotatey = 0;
	}

	// Aim Vector (in relation to cam/chasis)
	d.aimx = (in_cursorx-g_windowwidth/2)*UPM/(g_windowsquare/2);
	d.aimy = (g_windowheight/2-in_cursory)*UPM/(g_windowsquare/2);
	d.aimz = 1*UPM;
	d.aimlx = (lastcx-g_windowwidth/2)*UPM/(g_windowsquare/2);
	d.aimly = (g_windowheight/2-lastcy)*UPM/(g_windowsquare/2);

	lastcx = in_cursorx;
	lastcy = in_cursory;

	// Test
	d.testpress = in_keypress[SDLK_t];
	d.testpress2 = in_keypress[SDLK_y];
	if(d.testpress2) DEBUGON^=1;

	// Reset Press Variables
	input_reset();

	// Unlock Mutex
	thread_unlockinputmutex();


	return d;
}
//}}}


// FLAG MAKE ABOUT SECTION AND PUT THIS IN
//  two sets of functions: those that take raw input from SDL and transforms them into
//  internal intermediary format, and those that take intermediary data and transform it
//  into process input that's universal across input devices

