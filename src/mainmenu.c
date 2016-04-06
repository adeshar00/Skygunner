
// Includes
//{{{
#include <stdlib.h>

#define MAINMENU_C
#include "process.h"
#include "main.h"
#include "mainmenu.h"
#include "thread.h"
//}}}


// Main Menu functions for main thread
//{{{

 // Init
//{{{
void* mainmenu_init(void* input)
{

	// Initialize main menu data
	mainmenudata* m = (mainmenudata*)malloc(sizeof(mainmenudata));
	m->md = (maindata*)input;

	// Initialize inter-thread data
	mainmenuitd* mitd = (mainmenuitd*)malloc(sizeof(mainmenuitd));
	mitd->command = MMCOM_NULL;
	mitd->pass = 0;
	th_itd = (void*)mitd;

	// Allow UI to render menu (now that inter-thread data defined)
	thread_changegamephase(GAMEPHASE_MAINMENU);

	return (void*)m;
}
//}}}

 // Loop
//{{{
void mainmenu_loop(void* data)
{
	mainmenudata* m = (mainmenudata*)data;
	maindata* md = m->md;
	mainmenuitd* mitd = (mainmenuitd*)th_itd;
	// Don't need to bother with mutex calls since all data modification
	//  happens between waitforui and releaseui calls.

	thread_menuwaitforui();

	switch(mitd->command)
	{

		case MMCOM_NEWGAME:
			md->interstatedata = mitd->pass;
			// Instead of freeing ma_pass and mallocing interstate
			//  only to pass it the same data, just swapping the pointer. 
			mitd->pass = 0;
			md->state = MASTATE_NEWGAME;
			thread_menureleaseui();
			thread_changegamephase(GAMEPHASE_LOADING);
			process_pop(pr_mainstack);
			return;

		case MMCOM_TERMINATE:
			md->state = MASTATE_TERMINATE;
			thread_menureleaseui();
			thread_changegamephase(GAMEPHASE_LOADING);
			process_pop(pr_mainstack);
			return;

		case MMCOM_NULL:
			break;
	}

	// For if there's a case which doesn't return added later
	thread_menureleaseui();
}
//}}}

 // Pop
//{{{
void mainmenu_pop(void* data)
{
	// Changegamephase is in terminate portion of main loop,
	//  to make it obvious that it occurs after menu release;

	// Free main menu data
	mainmenudata* m = (mainmenudata*)data;
	free(m);

	// Free inter-thread data
	free(((mainmenuitd*)th_itd)->pass); // should be free anyway, but just to be safe
	free(th_itd);
	th_itd = 0;

}
//}}}

//}}}

// Main Menu functions for UI thread
//{{{

/*
   Since the option menu only modifies UI variables, it's handled
    exclusively by the UI.
   */

void mainmenu_newgame(int difficulty)
{
	mainmenuitd* mitd = (mainmenuitd*)th_itd;

	mitd->command = MMCOM_NEWGAME;

	// Store difficulty in pass, to hand off to main thread
	mitd->pass = malloc(sizeof(int));
	*(int*)(mitd->pass) = difficulty;

	thread_menuprocess();
}

void mainmenu_terminate()
{
	mainmenuitd* mitd = (mainmenuitd*)th_itd;
	mitd->command = MMCOM_TERMINATE;
	thread_menuprocess();
}

//}}}

