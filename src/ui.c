
// Includes
//{{{

#include <stdlib.h>
#include <math.h>

#define UI_C
#include "globals.h"
#include "ui.h"
#include "mainmenu.h"
#include "input.h"
#include "thread.h"
#include "process.h"
#include "render.h"
#include "sound.h"
#include "battle.h"
#include "shop.h"
#include "debug.h"

//}}}

// Prototypes
//{{{
void ui_battlechunkeffectadd(effectdata*,int,int,float,float,float);
void ui_battlechunkeffectremove(effectdata*,int);
//}}}


// UI Processes
//{{{

 // intro
//{{{
void* ui_intropush(void* input)
{
	int* ticks = (int*)malloc(sizeof(int));
	*ticks = 2;
	return (void*)ticks;
}
void ui_introloop(void* data)
{
	int* ticks = (int*)data;
	int temp = *ticks; // number of ticks that intro goes on for

	// Show Intro screen
	if(temp>0)
	{
		render_test(0xff0);
		/*
		printf("%c[2K", 27);
		printf("\r");
		switch(temp%4)
		{
			case 0:
				printf(" _ - INTRO! - _ ");
				break;
			case 1:
				printf(" - \\ INTRO! / - ");
				break;
			case 2:
				printf(" ^ | INTRO! | ^ ");
				break;
			case 3:
				printf(" - / INTRO! \\ - ");
				break;
		}
		fflush(stdout);
		*/
		thread_waitforuiticker();
		(*ticks)--;
	}
	else // Show "loading" screen while waiting for main thread to change phase
	{
		render_test(0x8f0);
		if(!thread_checkforgamephasechange())
		{
			thread_waitforuiticker();
		}
		else process_pop(pr_uistack);
	}

}
void ui_intropop(void* data)
{
	free(data);
}
process ui_introprocess = {ui_intropush,ui_introloop,ui_intropop};
//}}}

 // mainmenu
//{{{
typedef struct
{
	int menu;
} uimainmenudata;
void* ui_mainmenupush(void* input)
{
	uimainmenudata* m = (uimainmenudata*)malloc(sizeof(uimainmenudata));
	m->menu = 1;
	return (void*)m;
}
void ui_mainmenuloop(void* data)
{
	input_menu();
	uimainmenudata* m = (uimainmenudata*)data;
	if(!thread_checkforgamephasechange())
	{
		switch(m->menu)
		{
			case 1: // top
				render_test(0xf);
				in_keypress[49] = 1; // FLAG
				if(in_keypress[49])
				{
					m->menu = 3;
				}
				else if(in_keypress[50])
				{
					m->menu = 2;
				}
				else if(in_keypress[51])
				{
					mainmenu_terminate();
					m->menu = 0;
				}
				break;

			case 2: // options
				render_test(0xf0);
				m->menu = 1;
				/*
				switch(input)
				{
					case 1:
						printf("Enter new volume:\n");
						scanf("%d",&input);
						if(input>VOLMAX) input = VOLMAX;
						if(input<VOLMIN) input = VOLMIN;
						ui_volume = input;
						break;
					case 2:
						m->menu = 1;
						break;
					default:
						printf("Que?\n");
						break;
				}
				*/
				break;

			case 3: // new game
				render_test(0xff);
				in_keypress[49] = 1; // FLAG
				if(in_keypress[49])
				{
					m->menu = 0;
					mainmenu_newgame(0);
				}
				else if(in_keypress[50])
				{
					m->menu = 0;
					mainmenu_newgame(1);
				}
				else if(in_keypress[51])
				{
					m->menu = 1;
				}
				/*
				switch(input)
				{
					case 1:
						mainmenu_newgame(0);
						m->menu = 0;
						break;
					case 2:
						mainmenu_newgame(1);
						m->menu = 0;
						break;
					case 3:
						m->menu = 1;
						break;
					default:
						printf("Que?\n");
						break;
				}
				*/
				break;

		}
	}
	else
	{
		process_pop(pr_uistack);
	}

}
void ui_mainmenupop(void* data)
{
	free(data);
}
process ui_mainmenuprocess = {ui_mainmenupush,ui_mainmenuloop,ui_mainmenupop};
//}}}

 // loading
//{{{
void* ui_loadingpush(void* input)
{
	int* anim = (int*)malloc(sizeof(int));
	*anim = 0;
	return (void*)anim;
}
void ui_loadingloop(void* data)
{

	int* anim = (int*)data;

	// render loading
	switch(*anim)
	{
		case 0:
			render_test(0x0);
			break;
		case 1:
			render_test(0x555);
			break;
		case 2:
			render_test(0xaaa);
			break;
	}
	*anim = (*anim+1)%3;

	thread_waitforuiticker();

	if(thread_checkforgamephasechange())
	{
		process_pop(pr_uistack);
	}

}
void ui_loadingpop(void* data)
{
	free(data);
}
process ui_loadingprocess = {ui_loadingpush,ui_loadingloop,ui_loadingpop};
//}}}

 // battle
//{{{
typedef struct
{
	effectdata effects;
	int lastmaintickcount;
} uibattledata;
void* ui_battlepush(void* input)
{
	uibattledata* ub = (uibattledata*)malloc(sizeof(uibattledata));

	effectdata* e = &(ub->effects);

	e->chunkcount = 0;
	e->gascount = 0;
	
	// itd should be non-null and freshflag should be set to 1 in
	//  main battle process push, which should execute before
	//  ui battle process push

	return (void*)ub;
}
void ui_battleloop(void* data)
{

	// Check if gamephase has changed
	if(thread_checkforgamephasechange())
	{
		process_pop(pr_uistack);
		return;
	}


	// Variables
	uibattledata* ub = (uibattledata*)data;
	battleitd* bitd = (battleitd*)th_itd;

	//FLAGTE have renders be interpolated!
	// Check and set itd
	//{{{
	{
		// NOTE the rest of this function is able to access the render buffer
		//  portion of the ITD without having the mutex locked.
		// Only the ui thread is allowed to swap the buffers, and should
		//  only do this when the mutex is locked!
		// Make damn sure code Main thread doesn't touch render buffer data!!
		//  FLAG maybe make a function in here to handle data swap, and have it called from battle.c so it'd be impossible to have a fuckup (at least from there?)

		effectdata* e = &(ub->effects);
		int dtick = 0; // How many main ticks have happened since last render loop

		thread_lockitdmutex();

		// Read main thread data if fresh
		if(bitd->freshflag)
		{
			bitd->freshflag = 0;
			bitd->renderbufferid^= 1;
			battlerenderbuffer* brb = bitd->brb+bitd->renderbufferid;

			// Update 
			dtick = brb->maintickcount - ub->lastmaintickcount;
			ub->lastmaintickcount = brb->maintickcount;

			// Process new effect list
			// FLAGTE take outside of mutex lock?
			{
				neweffectlistnode* n = brb->neweffectlisthead;
				neweffectlistnode* next;
				while(n)
				{
					static int DELETEME = 0;
					//DEMO FLAG
					ui_battlechunkeffectadd(e, n->type, TPS/4,
							n->px,n->py,n->pz);
					next = n->next;
					free(n);
					n = next;
					DELETEME++; // FLAGTE do what he say! do what he saaaay!
				}
				brb->neweffectlisthead = 0;
				brb->neweffectlisttailpointer = &(brb->neweffectlisthead);
			}

			/* test that other buffer isn't showing
			battlerenderbuffer* brb = bitd->brb+(bitd->renderbufferid^1);
			brb->renderobjectcount=3;
			matrix testmat;
			matrix_identity(&testmat);
			brb->romodelnum[0] = 0;
			brb->romatrix[0] = testmat;
			matrix_translate(&testmat,1,1,0);
			brb->romodelnum[1] = 1;
			brb->romatrix[1] = testmat;
			matrix_translate(&testmat,0,-2,0);
			brb->romodelnum[2] = 0;
			brb->romatrix[2] = testmat;
			//bitd->freshflag = 1;
			*/
		}

		thread_unlockitdmutex();

		// Update effects FLAGTE clean up and make this not as wacky
		if(dtick>0)
		{

			for(;dtick>0;--dtick)
			{

				//xx

				int i;
				int ec = e->chunkcount;

				// Update effects
				for(i=0;i<ec;i++)
				{
					//debug_message("%d:%d ", e->chunktype[i], e->chunklife[i]);
					--e->chunklife[i];
				}

				// Check for deletions
				for(i=ec-1;i>=0;i--)
				{
					if(e->chunklife[i]<=0)
					{
						ui_battlechunkeffectremove(e, i);
					}
				}

			}


		}

	}
	//}}}

	// Render
	{
		intv mx, my;
		battlerenderbuffer* brb = bitd->brb+bitd->renderbufferid;
		float phase = (float)(thread_gettime()-brb->starttime);
		phase/= (float)BATTLEPERIOD*1000/UPM;
		//float tz = b->tlz + (b->tz-b->tlz)*phase;

		input_fetchmousecoords(&mx,&my);
		render_battle(brb,0/*tzFLAGTE*/,mx,my,&(ub->effects));
	}

	// Sound
	{
		//derp
	}

	// Sleep until end of tick
	thread_waitforuiticker();


}
void ui_battlepop(void* data)
{
	free(data);
}
process ui_battleprocess = {ui_battlepush,ui_battleloop,ui_battlepop};
//}}}

 // shop
//{{{
typedef struct
{
	int menu;
} uishopdata;
void* ui_shoppush(void* input)
{
	uishopdata* s = (uishopdata*)malloc(sizeof(uishopdata));
	s->menu = 1;

	// Refreshes ITD FLAG maybe should rename that function..., like refreshitd insead of getdaata
	shop_getdata();
	//printf("Welcome to the shop!\n");
	
	return (void*)s;
}
void ui_shoploop(void* data)
{

	uishopdata* s = (uishopdata*)data;
	//shopitd* sitd = (shopitd*)th_itd;

	switch(s->menu)
	{
		case 1:
			render_shop();
			//render_test(0xf80);
			input_menu();
			shop_getdata();
			if(in_keypress[49])
			{
				if(shop_buyammo())
				{
					// do some shit FLAG
				}
			}
			else if(in_keypress[50])
			{
				shop_leaveshop();
				s->menu = 0;
			}
			/*
			switch(input)
			{
				case 1:
					if(shop_buyammo())
						printf("Ding!\n");
					else
						printf("Not enough moonays...\n");
					break;
				case 2:
					shop_leaveshop();
					s->menu = 0;
					break;
			}
			*/
			break;
	}
	if(thread_checkforgamephasechange())
	{
		process_pop(pr_uistack);
	}
}
void ui_shoppop(void* data)
{
	free(data);
}
process ui_shopprocess = {ui_shoppush,ui_shoploop,ui_shoppop};
//}}}

 // gameover
//{{{
void* ui_gameoverpush(void* input)
{
	int* anim = (int*)malloc(sizeof(int));
	*anim = 20;
	return (void*)anim;
}
void ui_gameoverloop(void* data)
{
	int* anim = (int*)data;
	*anim = 0;//FLAG just to make short

	if(*anim>0)
	{
		render_test(0xfff);
		/* FLAG
		printf("%c[2K", 27);
		printf("\r");
		switch(*anim%4)
		{
			case 0:
				printf(" = - GAME OVER! - = ");
				break;
			case 1:
				printf(" - \\ GAME OVER! / - ");
				break;
			case 2:
				printf(" = | GAME OVER! | = ");
				break;
			case 3:
				printf(" - / GAME OVER! \\ - ");
				break;
		}
		fflush(stdout);
		*/
		thread_waitforuiticker();
		(*anim)--;
	}
	else
	{
		if(thread_checkforgamephasechange()) process_pop(pr_uistack);
	}

}
void ui_gameoverpop(void* data)
{
	free(data);
}
process ui_gameoverprocess = {ui_gameoverpush,ui_gameoverloop,ui_gameoverpop};
//}}}

 // terminate 
//{{{
void ui_terminateloop(void* data)
{

	render_test(0xf0f);
	render_deinit();
	thread_terminateui();
}
process ui_terminateprocess = {ui_nullpush,ui_terminateloop,ui_null};
//}}}

 // core
//{{{
void ui_coreloop(void* data)
{
	process* procmap[GAMEPHASE_TERMINATE+1];
	procmap[GAMEPHASE_INTRO] = &ui_introprocess;
	procmap[GAMEPHASE_MAINMENU] = &ui_mainmenuprocess;
	procmap[GAMEPHASE_LOADING] = &ui_loadingprocess;
	procmap[GAMEPHASE_BATTLE] = &ui_battleprocess;
	procmap[GAMEPHASE_SHOP] = &ui_shopprocess;
	procmap[GAMEPHASE_GAMEOVER] = &ui_gameoverprocess;
	procmap[GAMEPHASE_TERMINATE] = &ui_terminateprocess;

	process_push(pr_uistack, *procmap[thread_getgamephase()], 0);
}
process ui_coreprocess = {ui_nullpush,ui_coreloop,ui_null};
//}}}

//}}}

// UI Functions
//{{{

 // UI thread init
//{{{
void ui()
{

	render_init();
	render_modelinit(); // FLAG maybe figure out some way to put into main thread
	input_init();

	ui_volume = VOLMAX;

	pr_uistack = process_createstack();
	process_push(pr_uistack, ui_coreprocess, 0);

	thread_uiloop();
}
//}}}

 // Battle effect functions
//{{{

  // Add effect
//{{{
void ui_battlechunkeffectadd(effectdata* e, int type, int life, float x, float y, float z)
{
	if(e->chunkcount>=MAXCHUNKEFFECTS)
	{
		debug_errormessage("Chunk effect spawn failure: max chunk effects reached.\n");
		return;
	}

	//xx
	int ec = e->chunkcount;
	e->chunktype[ec] = type;
	e->chunkpx[ec] = x;
	e->chunkpy[ec] = y;
	e->chunkpz[ec] = z;
	e->chunklife[ec] = life;

	++e->chunkcount;
}
//}}}

  // Remove effect
//{{{
void ui_battlechunkeffectremove(effectdata* e, int index)
{
	int top = e->chunkcount-1;

	if(index<0 || index>top)
	{
		debug_errormessage("Chunk effect removal error; invalid index passed.\n");
		return;
	}

	e->chunktype[index] = e->chunktype[top];
	e->chunklife[index] = e->chunklife[top];
	e->chunkpx[index] = e->chunkpx[top];
	e->chunkpy[index] = e->chunkpy[top];
	e->chunkpz[index] = e->chunkpz[top];

	--e->chunkcount;
}
//}}}

//}}}

//}}}

