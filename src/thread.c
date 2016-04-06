
//====== IF MULTITHREADED ========================================================================//
//{{{
#ifdef EMSCRIPTEN
#define SINGLETHREADED
#endif
#ifndef SINGLETHREADED
//}}}



// Includes
//{{{

#include <pthread.h>

#include <time.h>
#include <stdlib.h>
#include <unistd.h> // FLAG for usleep, delete? is usleep below necessary?

#define THREAD_C
#include "globals.h"
#include "thread.h"
#include "process.h"
#include "ui.h"
#include "debug.h"

//}}}

// Variables Structs & Constants
//{{{

pthread_t th_uithread;

void* th_itd; // pointer to interthread data (see ABOUT.txt for more info on ITD)
pthread_mutex_t th_itdmutex; // mutex for ITD

// FLAG maybe merge below sets of mutexes and conds? use should never overlap...
// Gamephase Variables
int th_gamephase;
pthread_cond_t th_changecond;
pthread_mutex_t th_changecondmutex;
pthread_cond_t th_changeconfirm;
pthread_mutex_t th_changeconfirmmutex;

// Menu Variables
pthread_cond_t th_menucond;
pthread_mutex_t th_menumutex;
pthread_cond_t th_menureleasecond;
pthread_mutex_t th_menureleasemutex;

// Input mutex
pthread_mutex_t th_inputmutex;

// Ticker struct (see "ticker functionality" fold for more info)
typedef struct
{
	int period;
	int lasttime;
	int lag;
	int maxbuffer;
} realticker;

typedef void* ticker;
ticker thread_createticker(int, int);
void thread_waitforticker(ticker);
void thread_restartticker(ticker);
void thread_destroyticker(ticker);
int thread_gettime();

ticker th_battleticker;
ticker th_uiticker;
	
//}}}


// Functions for main.c
//{{{

void thread_init()
{
	th_itd = 0;
	pthread_mutex_init(&th_itdmutex, 0);

	th_battleticker = thread_createticker(BATTLEPERIOD*1000/UPM,BATTLEBUFFER*1000/UPM);
	th_uiticker = thread_createticker(UIPERIOD,UIBUFFER);

	th_gamephase = GAMEPHASE_INTRO;

	pthread_mutex_init(&th_inputmutex, 0);

	pthread_cond_init(&th_changecond, 0);
	pthread_mutex_init(&th_changecondmutex, 0);
	pthread_cond_init(&th_changeconfirm, 0);
	pthread_mutex_init(&th_changeconfirmmutex, 0);
	pthread_cond_init(&th_menucond, 0);
	pthread_mutex_init(&th_menumutex, 0);
	pthread_cond_init(&th_menureleasecond, 0);
	pthread_mutex_init(&th_menureleasemutex, 0);
	
	pthread_mutex_lock(&th_changecondmutex);
	pthread_mutex_lock(&th_menumutex);
}

void thread_startui()
{
	// Creates the UI thread
	pthread_create(&th_uithread, 0, (void*)&ui, 0);
}

void thread_startmainloop()
{
	while(1)
		process_toploop(pr_mainstack);
}

void thread_terminatemain()
{
	pthread_join(th_uithread, 0);

	process_destroystack(pr_mainstack);

	thread_destroyticker(th_battleticker);

	pthread_cond_destroy(&th_changecond);
	pthread_mutex_destroy(&th_changecondmutex);
	pthread_cond_destroy(&th_changeconfirm);
	pthread_mutex_destroy(&th_changeconfirmmutex);
	pthread_cond_destroy(&th_menucond);
	pthread_mutex_destroy(&th_menumutex);
	pthread_cond_destroy(&th_menureleasecond);
	pthread_mutex_destroy(&th_menureleasemutex);

	pthread_exit(0);
}

void thread_changegamephase(int phase)
{

	// See "ABOUT.txt" for details on the use of this function


	// Wait for UI thread to run checkforgamephasechange
	pthread_cond_wait(&th_changecond, &th_changecondmutex);

	// Now that UI thread is ready, change game phase
	th_gamephase = phase;

	// Send signal to confirm that game phase is now changed
	pthread_mutex_lock(&th_changeconfirmmutex);
	pthread_cond_signal(&th_changeconfirm);
	pthread_mutex_unlock(&th_changeconfirmmutex);
}

//}}}

// Functions for ui.c
//{{{

void thread_uiloop()
{

	// Iterates when gamephase changes
	while(1)
	{
		process_toploop(pr_uistack);
	}

}

int thread_checkforgamephasechange()
{

	// Returns 1 if changed, 0 if unchanged
	// See "ABOUT.txt" for details on the use of this function

	// If mutex unlocked, it means main thread is waiting to change phase
	if(!pthread_mutex_trylock(&th_changecondmutex))
	{

		// Lock mutex for confirmation signal
		pthread_mutex_lock(&th_changeconfirmmutex);

		// Send signal so main thread can proceed with phase change
		pthread_cond_signal(&th_changecond);
		pthread_mutex_unlock(&th_changecondmutex);

		// Wait for confirmation signal from main thread
		pthread_cond_wait(&th_changeconfirm, &th_changeconfirmmutex);
		pthread_mutex_unlock(&th_changeconfirmmutex);

		// Return that the game phase has changed
		return 1;
	}

	// Mutex was locked: return that the game phase is unchanged
	return 0;

}

void thread_waitforuiticker()
{
	thread_waitforticker(th_uiticker);
}

int thread_getgamephase()
{
	// Returns gamephase

	int phase;
	phase = th_gamephase;
	return phase;
}

void thread_terminateui()
{
	// Exits UI thread

	thread_destroyticker(th_uiticker);

	process_destroystack(pr_uistack);

	pthread_exit(0);
}

//}}}

// Functions for Interthread data (ITD)
//{{{
void thread_lockitdmutex()
{
		pthread_mutex_lock(&th_itdmutex);
}
void thread_unlockitdmutex()
{
		pthread_mutex_unlock(&th_itdmutex);
}
//}}}

// Functions for Input Mutex
//{{{
void thread_lockinputmutex()
{
		pthread_mutex_lock(&th_inputmutex);
}
void thread_unlockinputmutex()
{
		pthread_mutex_unlock(&th_inputmutex);
}
//}}}

// Functions for menu-related thread syncing
//{{{

/*
   When the game is displaying a menu, the main thread waits for the UI thread
     to recieve input from the user.
    When input is received, the UI thread notifies the main thread and passes it
     the necessary data, and then waits until the main thread has processed
     the input.
    This protocol is carried out via the use of the below three functions:
     waitforui is called to hang the main thread,
     menuprocess is called from the ui thread to unhang the main thread
      and then hang itself until the main thread is done,
     releaseui is called from the main thread when processing is finished
      and the ui thread can be unhung.

   Note: Though thread_menuprocess is meant to be called from the UI thread, it should
    not be called directly in ui.c or any of it's child .c files.
    Instead, it should be called from external functions (defined in the .c files that
    provide main thread functionality) written to be called from UI thread.

    All code execution between waitforui and releaseui should be virtually instantaneous,
     as the UI thread is hung until releaseui is called (and graphics will freeze)
    Changegamephase should NOT be called between wait and release, or game will
     hang indefinitely.

   */

void thread_menuwaitforui()
{
	// Hangs main thread until UI thread requests processing (via thread_menuprocess)

	pthread_cond_wait(&th_menucond, &th_menumutex);

}

void thread_menureleaseui()
{
	// This thread is called when the main thread is done with UI thread's request
	// Allows UI thread's call to thread_menuprocess to return

	pthread_mutex_lock(&th_menureleasemutex);
	pthread_cond_signal(&th_menureleasecond);
	pthread_mutex_unlock(&th_menureleasemutex);
}

void thread_menuprocess()
{
	// Called from UI thread.  Allows main thread to proceed and process request.
	// Hangs UI thread until main thread has finished processing request and
	//  calls "thread_menureleaseui".

	pthread_mutex_lock(&th_menumutex);

	pthread_mutex_lock(&th_menureleasemutex);
	pthread_cond_signal(&th_menucond);
	pthread_mutex_unlock(&th_menumutex);
	pthread_cond_wait(&th_menureleasecond, &th_menureleasemutex);

	pthread_mutex_unlock(&th_menureleasemutex);
}

//}}}

// Battle ticker functions
//{{{

void thread_restartbattleticker()
{
	thread_restartticker(th_battleticker);
}

void thread_waitforbattleticker()
{
	thread_waitforticker(th_battleticker);
}

int thread_getbattleperiod()
{
	return BATTLEPERIOD*1000/UPM;
}

//}}}

// Functions for tickers
//{{{

/*
    Tickers were initially made to be used outside of this file, so if anything
     seems odd (like using a typedef to hide the ticker struct), that's why

    About tickers:
     Certain routines need to be run at an average interval of so many milliseconds.
     When called, the "thread_waitforticker" function will hang the calling function
      for a set period of time, which will result in the callin routine running at the
      desired interval (on average).
     A ticker struct is used to keep track of the data pertinent to the calling loop,
      included the interval it should produce.
     Tickers are created by calling the thread_createticker function, and passing
      the desired interval in milliseconds as the argument.

    The "ticker" in tick.h acts as a way to hide how a ticker struct
     actually looks.  The "ticker" is just a void pointer, while the
     "realticker" struct represents a true ticker
   */


void thread_errorcheck(ticker tpointer) // internal
{
	if(tpointer==0)
	{
		debug_errormessage("Null pointer passed to tick function.\n");
		exit(3);
	}
}

int thread_gettime()
{
	// Returns time in ms since game genesis

	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec*1000 + t.tv_nsec/1000000;
}

ticker thread_createticker(int period, int maxbuffer)
{
	// Creates a real ticker, and passes a pointer ("ticker") back to calling function
	// Period is in milliseconds

	realticker* newticker;
	newticker = (realticker*)malloc(sizeof(realticker));
	newticker->lag = 0;
	newticker->lasttime = thread_gettime();
	newticker->period = period;
	newticker->maxbuffer = maxbuffer;
	return (ticker)newticker;
}

void thread_waitforticker(ticker tpointer)
{
	// Calling this function hangs until the end of the current tick

	int waketime;
	int temptime;
	realticker* t;

	thread_errorcheck(tpointer);
	t = (realticker*)tpointer;

	// Calculate wake time
	waketime = t->lasttime + t->period - t->lag;

	// Calculate time to sleep between now and wake time
	temptime = (waketime - thread_gettime())*999; // time in microseconds, minus small amount

	// Hang until wait time
	if(temptime>0)
		usleep(temptime);
	while((temptime=thread_gettime())<waketime){}

	// Determine how much lag, if any.  Add to buffer
	t->lag+= (temptime - t->lasttime) - t->period;
	if(t->lag > t->maxbuffer) t->lag = t->maxbuffer;

	// Record wake time for next tick
	t->lasttime = temptime;

}

void thread_restartticker(ticker tpointer)
{
	// Resets lasttime and cancels lag; for use when a ticker has been interupted
	//  (e.x. resuming after pausing a game)

	thread_errorcheck(tpointer);
	realticker* t = (realticker*)tpointer;
	t->lasttime = thread_gettime();
	t->lag = 0;
}

void thread_destroyticker(ticker tpointer)
{
	thread_errorcheck(tpointer);
	free(tpointer);
	tpointer = 0;
}

//}}}




//===== IF SINGLE THREADED =======================================================================//
//{{{
#else
//}}}



// Includes
//{{{

#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#define THREAD_C
#include "intmath.h"
#include "globals.h"
#include "thread.h"
#include "process.h"
#include "ui.h"
#include "debug.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

//}}}

// Variables Structs & Constants
//{{{

int th_gamephase;

void* th_itd; // pointer to interthread data (see ABOUT.txt for more info on ITD)

#define PERIOD (BATTLEPERIOD*1000/UPM)	// How many milliseconds between ticks FLAG put in seconds?
#define BUFFER PERIOD/2		// Max lag cusion for ticker, see tick.c for more info

// Ticker struct (see "ticker functionality" fold for more info)
typedef struct
{
	int period;
	int lasttime;
	int lag;
	int maxbuffer;
} realticker;

typedef void* ticker;
ticker thread_createticker(int, int);
void thread_waitforticker(ticker);
void thread_restartticker(ticker);
void thread_destroyticker(ticker);
int thread_gettime();

ticker th_ticker;

int th_gamephasefauxmutex; // acts as a way to communicate game phase changes between processes
// 0 means game phase is same, 1 means main process set to change and ui yet to aknowledge
	
//}}}


// Functions for main.c
//{{{

void thread_init()
{
	th_itd = 0;

	th_ticker = thread_createticker(PERIOD,BUFFER);

	th_gamephase = GAMEPHASE_INTRO;
	th_gamephasefauxmutex = 0;

}

void thread_startui()
{
	// Creates the UI thread
	ui();
}

void thread_mainloop()
{
	// Main thread code
	process_toploop(pr_mainstack);

	// UI thread code
	process_toploop(pr_uistack);
}
void thread_startmainloop()
{

#ifdef EMSCRIPTEN
	emscripten_set_main_loop(thread_mainloop, 1000/PERIOD, 1);//FLAG make so uses defined frequency
#else
	while(1)
	{
		thread_mainloop();

		// Actual ticker wait
		//  (ticker calls in threads just call empty functions when single threaded)
		thread_waitforticker(th_ticker);
	}
#endif
}

void thread_terminatemain()
{

	thread_destroyticker(th_ticker);

	exit(0);
}

void thread_changegamephase(int phase)
{
	th_gamephase = phase;
	th_gamephasefauxmutex = 1;
}

//}}}

// Functions for ui.c
//{{{

void thread_uiloop()
{


}

int thread_checkforgamephasechange()
{
	if(th_gamephasefauxmutex)
	{
		th_gamephasefauxmutex = 0;
		return 1;
	}
	return 0;
}

void thread_waitforuiticker()
{

}

int thread_getgamephase()
{
	// Returns gamephase

	int phase;
	phase = th_gamephase;
	return phase;
}

void thread_terminateui()
{

}

//}}}

// Functions for Interthread data (ITD)
//{{{
void thread_lockitdmutex()
{

}
void thread_unlockitdmutex()
{

}
//}}}

// Functions for Input Mutex
//{{{
void thread_lockinputmutex(){}
void thread_unlockinputmutex(){}
//}}}

// Functions for menu-related thread syncing
//{{{

void thread_menuwaitforui()
{

}

void thread_menureleaseui()
{

}

void thread_menuprocess()
{
		process_toploop(pr_mainstack);
}

//}}}

// Battle ticker functions
//{{{

void thread_restartbattleticker()
{

}

void thread_waitforbattleticker()
{

}

int thread_getbattleperiod()
{
	return PERIOD;
}

//}}}

// Functions for tickers
//{{{

/*
    Tickers were initially made to be used outside of this file, so if anything
     seems odd (like using a typedef to hide the ticker struct), that's why

    About tickers:
     Certain routines need to be run at an average interval of so many milliseconds.
     When called, the "thread_waitforticker" function will hang the calling function
      for a set period of time, which will result in the callin routine running at the
      desired interval (on average).
     A ticker struct is used to keep track of the data pertinent to the calling loop,
      included the interval it should produce.
     Tickers are created by calling the thread_createticker function, and passing
      the desired interval in milliseconds as the argument.

    The "ticker" in tick.h acts as a way to hide how a ticker struct
     actually looks.  The "ticker" is just a void pointer, while the
     "realticker" struct represents a true ticker

    (Note: put this sytem in place before learning that you could declare
     a struct without showing it's guts... I know, it's bad)
   */


void thread_errorcheck(ticker tpointer) // internal
{
	if(tpointer==0)
	{
		debug_errormessage("Null pointer passed to tick function.\n");
		exit(3);
	}
}

int thread_gettime()
{
	// Returns time in ms since game genesis

	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec*1000 + t.tv_nsec/1000000;
}

ticker thread_createticker(int period, int maxbuffer)
{
	// Creates a real ticker, and passes a pointer ("ticker") back to calling function
	// Period is in milliseconds

	realticker* newticker;
	newticker = (realticker*)malloc(sizeof(realticker));
	newticker->lag = 0;
	newticker->lasttime = thread_gettime();
	newticker->period = period;
	newticker->maxbuffer = maxbuffer;
	return (ticker)newticker;
}

void thread_waitforticker(ticker tpointer)
{
	// Calling this function hangs until the end of the current tick

	int waketime;
	int temptime;
	realticker* t;

	thread_errorcheck(tpointer);
	t = (realticker*)tpointer;

	// Calculate wake time
	waketime = t->lasttime + t->period - t->lag;

	// Calculate time to sleep between now and wake time
	temptime = (waketime - thread_gettime())*999; // time in microseconds, minus small amount

	// Hang until wait time
	if(temptime>0)
		usleep(temptime);
	while((temptime=thread_gettime())<waketime){}

	// Determine how much lag, if any.  Add to buffer
	t->lag+= (temptime - t->lasttime) - t->period;
	if(t->lag > t->maxbuffer) t->lag = t->maxbuffer;

	// Record wake time for next tick
	t->lasttime = temptime;

}

void thread_restartticker(ticker tpointer)
{
	// Resets lasttime and cancels lag; for use when a ticker has been interupted
	//  (e.x. resuming after pausing a game)

	thread_errorcheck(tpointer);
	realticker* t = (realticker*)tpointer;
	t->lasttime = thread_gettime();
	t->lag = 0;
}

void thread_destroyticker(ticker tpointer)
{
	thread_errorcheck(tpointer);
	free(tpointer);
	tpointer = 0;
}

//}}}




//================================================================================================//
//{{{
#endif
//}}}



