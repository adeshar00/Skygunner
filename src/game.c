
// Includes
//{{{

#include <stdlib.h>

#define GAME_C
#include "intmath.h"
#include "process.h"
#include "game.h"
#include "thread.h"
#include "battle.h"
#include "shop.h"
#include "debug.h"

//}}}


// Game Process
//{{{

 // Init
//{{{
void* game_init(void* input)
{


	int difficulty = *(int*)input;
	free(input);

	// Create game data
	gamedata* g = (gamedata*)malloc(sizeof(gamedata));
	g->level = 1;

	// Populate game data according to difficulty
	switch(difficulty)
	{
		case 0:
			g->money = 0;
			g->ammo = 500;
			g->state = GAMESTATE_BATTLE;
			break;

		case 1:
			g->money = 1000;
			g->ammo = 100;
			g->state = GAMESTATE_SHOP;
			break;
	}

	return (void*)g;
}
//}}}

 // Loop
//{{{
void game_loop(void* data)
{

	gamedata* g = (gamedata*)data;

	gamemod* gm;

	switch(g->state)
	{
		case GAMESTATE_BATTLE:
			gm = (gamemod*)malloc(sizeof(gamemod));

			// FLAG
			// Legacy code from pre-OGL: leave in place until shop system
			//  is in place, then scrap all of it
			gm->ammo = &(g->ammo);
			gm->money = &(g->money);
			gm->returnvalue = &(g->returnvalue);
			gm->level = g->level;
			gm->ars = (arsenal*)malloc(sizeof(arsenal));

			// Generate temporary arsenal for use in battle phase
			{
				int guns = 3;
				int aboxes = 4;
				gm->ars->guncount = guns;
				gm->ars->ammoboxcount = aboxes;
				gm->ars->guntype = (int*)malloc(sizeof(int)*guns);
				gm->ars->gunammobox = (int*)malloc(sizeof(int)*guns);
				gm->ars->gunphase = (intv*)malloc(sizeof(intv)*guns);
				gm->ars->ammoboxmissletype = (int*)malloc(sizeof(int)*aboxes);
				gm->ars->ammoboxammo = (int*)malloc(sizeof(int)*aboxes);


				int i=0;//vulcan
				gm->ars->guntype[i] = 0;
				gm->ars->gunammobox[i] = 0;
				gm->ars->gunphase[i] = 0;
				i++;// charge cannon
				gm->ars->guntype[i] = 1;
				gm->ars->gunammobox[i] = 1;
				gm->ars->gunphase[i] = 0;
				i++;// semiauto test rifle
				gm->ars->guntype[i] = 2;
				gm->ars->gunammobox[i] = 0;
				gm->ars->gunphase[i] = 0;
				i++;

				//xx FLAGTE
				if(i!=guns)debug_errormessage("derp gun count in arsenal off\n");

				i=0;//bullets
				gm->ars->ammoboxmissletype[i] = 0;
				gm->ars->ammoboxammo[i] = 20000;
				i++;//chargecells
				gm->ars->ammoboxmissletype[i] = 1;
				gm->ars->ammoboxammo[i] = 500;
				i++;
				gm->ars->ammoboxmissletype[i] = 0;
				gm->ars->ammoboxammo[i] = 3000500;
				i++;
				gm->ars->ammoboxmissletype[i] = 2;
				gm->ars->ammoboxammo[i] = 666666;
				i++;

				if(i!=aboxes)debug_errormessage("derp box count in arsenal off\n");
			}

			// Push battle process to top of process stack
			process_push(pr_mainstack, battleprocess, (void*)gm);
			// Set state to "return" for the next game loop (when battle pops)
			g->state = GAMESTATE_RETURN;
			break;

		case GAMESTATE_SHOP:
			gm = (gamemod*)malloc(sizeof(gamemod));
			gm->money = &(g->money);
			gm->ammo = &(g->ammo);
			// Push shop process to top of process stack
			process_push(pr_mainstack, shopprocess, (void*)gm);
			// Set state to "battle" for the next game loop (when shop pops)
			g->state = GAMESTATE_BATTLE;
			break;
			
		case GAMESTATE_RETURN:
			// FLAG free gm, as well as it's dynamic arrays (arsenal)
			// This section runs when battle process pops
			if(g->returnvalue) // If player survived battle:
			{
				g->level++;
				g->state = GAMESTATE_SHOP;
			}
			else // If player died
			{
				g->state = GAMESTATE_FIN;
			}
			break;

		case GAMESTATE_FIN:
				process_pop(pr_mainstack);
			break;
	}
}
//}}}

 // Pop
//{{{
void game_pop(void* data)
{

	thread_changegamephase(GAMEPHASE_GAMEOVER);

	thread_changegamephase(GAMEPHASE_LOADING);

	gamedata* g = (gamedata*)data;
	free(g);

}
//}}}

//}}}

