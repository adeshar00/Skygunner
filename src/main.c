
// Includes
//{{{

#include <stdlib.h>

#define MAIN_C
#include "globals.h"
#include "process.h"
#include "main.h"
#include "mainmenu.h"
#include "thread.h"
#include "game.h"
#include "init.h"

//}}}


// Main Functions
//{{{

 // main()
//{{{
int main(int argc, char* argv[])
{

	// Initialize global variables
	globals_init();

	// Initialize thread data (inits mutexes and conds and sets gamephase to "intro")
	thread_init();

	// Create UI thread (which jumps right into intro routine on creation)
	thread_startui();

	// Initialize main thread process stack and push mainprocess
	pr_mainstack = process_createstack();
	process_push(pr_mainstack, mainprocess, 0);

	// Begin process loop
	thread_startmainloop();

	return 0; // Never executed, just here to make the compiler shut up
}
//}}}

 // Init
//{{{
void* main_init(void* input)
{

	// Declare main data
	maindata* m = (maindata*)malloc(sizeof(maindata));

	// Initialize main data
	m->state = MASTATE_MENU;
	m->interstatedata = 0;

	// Load assets
	init_init();

	// Signal UI thread to end INTRO phase
	thread_changegamephase(GAMEPHASE_LOADING);

	return (void*)m;
}
//}}}

 // Loop
//{{{
void main_loop(void* data)
{

	/* Game Phase should always be LOADING at this point.  This process acts
	    as kind of an intermediary process between the intro phase, the menu phase,
	    a new game, and termination. Which way it goes depends on what m->state is set to:
	    m->state will have been set during main_init, by the mainmenu
	    process, or within this function itself before pushing a new process.
	   */


	// Create pointer to main data
	maindata* m = (maindata*)data;

	// Process the predetermined state
	switch(m->state)
	{

		case MASTATE_MENU:
			process_push(pr_mainstack, mainmenuprocess, (void*)m);
			// The mainmenu process has access to m->state, and will
			//  modify it so that when the mainmenu process is popped,
			//  this function will begin with a new value for m->state.
			break;

		case MASTATE_NEWGAME:
			{
				int difficulty;
				int* gameinput;
				// Grab difficulty set by mainmenu, and pass it on to
				//  gameprocess when pushed
				difficulty = *(int*)(m->interstatedata);
				gameinput = (int*)malloc(sizeof(int));
				*gameinput = difficulty;
				free(m->interstatedata);
				m->interstatedata = 0;
				// Set state to menu for when gameprocess pops
				m->state = MASTATE_MENU;
				process_push(pr_mainstack, gameprocess,(void*)gameinput);
			}
			break;

		case MASTATE_TERMINATE:
			thread_changegamephase(GAMEPHASE_TERMINATE);
			thread_terminatemain();
			break;

	}
}
//}}}

//}}}

