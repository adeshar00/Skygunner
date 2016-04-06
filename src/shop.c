
// Includes
//{{{

#include <stdlib.h>

#define SHOP_C
#include "intmath.h"
#include "battle.h"
#include "game.h"
#include "process.h"
#include "shop.h"
#include "thread.h"

//}}}


// Shop Functions (main thread)
//{{{

 // Init
//{{{
void* shop_init(void* input)
{
	// Allocate inter-thread data
	th_itd = malloc(sizeof(shopitd));
	shopitd* sitd = (shopitd*)th_itd;

	sitd->command = SH_NULL;
	sitd->pass = 0;

	// Once inter-thread data exists, signal UI to begin SHOP phase
	thread_changegamephase(GAMEPHASE_SHOP);

	// Input is a gamemod struct, which is the only data the shop process needs
	return input;
}
//}}}

 // Loop
//{{{
void shop_loop(void* data)
{

	/* Waitforui() always returns 1 if multithreaded.  If single threaded, returns 1
	   if there's menu input to be processed, else returns 0.
	   */


	gamemod* gm = (gamemod*)data;
	shopitd* sitd = (shopitd*)th_itd;


	thread_menuwaitforui();
	// Don't need to bother with mutex calls since all data modification
	//  happens between waitforui and releaseui calls.
	switch(sitd->command)
	{

		case SH_BUYAMMO:
			sitd->pass = malloc(sizeof(int));
			if(*(gm->money)>=300)
			{
				*(gm->money)-= 300;
				*(gm->ammo)+= 150;
				*(int*)sitd->pass = 1;
			}
			else
			{
				*(int*)sitd->pass = 0;
			}
			sitd->command = SH_NULL;
			break;

		case SH_GETDATA:
			sitd->money = *(gm->money);
			sitd->ammo = *(gm->ammo);
			sitd->command = SH_NULL;
			break;

		case SH_QUIT:
			// Pop calls Changethreadphase, so releasing UI first
			thread_menureleaseui();
			process_pop(pr_mainstack);
			return;
	}

	thread_menureleaseui();

}
//}}}

 // Pop
//{{{
void shop_pop(void* data)
{
	// Destroy gamemod struct
	free(data);

	// Destroy shop itd
	free(((shopitd*)th_itd)->pass);
	free(th_itd);
	th_itd = 0;

	// Change phase
	thread_changegamephase(GAMEPHASE_LOADING);
}
//}}}

//}}}

// Functions for UI menu
//{{{

int shop_buyammo()
{
	// Returns 1 if purchase successful, 0 if not

	shopitd* sitd = (shopitd*)th_itd;

	int r;
	sitd->command = SH_BUYAMMO;
	thread_menuprocess();

	r = *((int*)(sitd->pass));
	free(sitd->pass);
	sitd->pass = 0;
	return r;
}

void shop_leaveshop()
{
	shopitd* sitd = (shopitd*)th_itd;
	sitd->command = SH_QUIT;
	thread_menuprocess();
}

void shop_getdata() // FLAG remove/replace this function when basic io interface removed?
{
	// FLAG is pass pointer necessary? delete?
	shopitd* sitd = (shopitd*)th_itd;
	sitd->command = SH_GETDATA;
	thread_menuprocess();
}

//}}}
