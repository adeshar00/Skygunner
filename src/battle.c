
// Includes
//{{{

#include <stdlib.h>

#define BATTLE_C
#include "globals.h"
#include "game.h"	// include first for parent structs
#include "process.h"
#include "input.h"
#include "thread.h"
#include "battle.h"
#include "intmath.h"
#include "debug.h"
#include "render.h"

//}}}

// Prototypes
//{{{
void battle_updateitd(battleitd*,battledata*);
void battle_kinematics(battledata*);
void battle_dynamics(battledata*);
void battle_enemyadd(enemydata*,intv,intv,intv,inta,inta,int);
void battle_enemydelete(enemydata*,int);
void battle_missleadd(missledata*,intv,intv,intv,intv,intv,intv,int,intv);
void battle_missledelete(missledata*,int);
void battle_shootmissle(battledata*,inputdatabattle*,int,int,intv);
void battle_grauto(battledata*,inputdatabattle*,int);
void battle_grcharge(battledata*,inputdatabattle*,int);
void battle_grsemi(battledata*,inputdatabattle*,int);
void battle_initgr();
void battle_initWOWZERS();
//}}}


// Battle Process
//{{{

 // Init
//{{{
void* battle_init(void* input)
{

	battledata* b = (battledata*)malloc(sizeof(battledata));

	// Initialize battle data
	{

		b->copterpx = -10*UPM; // FLAG eventually base on gamemod
		b->copterpy = 0;
		b->copterpz = 10*UPM;
		b->coptervx = 0;
		b->coptervy = 0;
		b->coptervz = 0;
		b->equippedgun = 0;

		b->camx = b->copterpx;
		b->camy = b->copterpy;
		b->camz = b->copterpz;
		b->camtheta = 0;
		b->camphi = 0;
		b->camlx = b->camx;
		b->camly = b->camy;
		b->camlz = b->camz;
		b->camltheta = b->camtheta;
		b->camlphi = b->camphi;

		b->gm = (gamemod*)input;
		b->md.misslecount = 0;
		b->ed.enemycount = 0;

		b->neweffectlisthead = 0;
		b->neweffectlisttailpointer = &(b->neweffectlisthead);

		b->tickcount = 0;

	}

	// Create and set ITD
	{
		th_itd = malloc(sizeof(battleitd));
		battleitd* bitd = (battleitd*)th_itd;
		bitd->brb[0].neweffectlisttailpointer = &(bitd->brb[0].neweffectlisthead);
		bitd->brb[1].neweffectlisttailpointer = &(bitd->brb[1].neweffectlisthead);
		battle_updateitd(bitd, b);
	}


	// Create benchmarks
	debug_benchcreate("Battle");
	debug_benchcreate("Kinematic");
	debug_benchcreate("Dynamics");
	debug_benchcreate("Test1");
	debug_benchcreate("Test2");
	debug_benchcreate("Render");
	debug_benchcreate("Update");


	// Begin Battle Loop (all ITD data should be set before this is called!)
	thread_changegamephase(GAMEPHASE_BATTLE);

	// Restart battle ticker
	thread_restartbattleticker();

	return (void*)b;
}
//}}}

 // Loop
//{{{
void battle_loop(void* data)
{

	debug_benchstart("Battle");

	// Variables
	battledata* b = (battledata*)data;
	battleitd* bitd = (battleitd*)th_itd;

	// Update visual data
	thread_lockitdmutex();
	battle_updateitd(bitd, b);
	thread_unlockitdmutex();

	// Move game objects
	debug_benchstart("Kinematic");
	battle_kinematics(b);
	debug_benchstop("Kinematic","");

	// Process interactions between game objects
	debug_benchstart("Dynamics");
	battle_dynamics(b);
	debug_benchstop("Dynamics","");

	// Output Benchmark Tally if it's ready
	debug_benchstop("Battle","Test poopy %d",7);
	debug_benchtally();

	// Check if battle over and tick hang FLAG reword this...
	if(0) // PUT BATTLE END CONDITIONS HERE (player death, all enemies dead, etc)
	{
		//process_pop(pr_mainstack);
	}
	else
	{
		thread_waitforbattleticker();
	}
	
	++b->tickcount;

}
//}}}

 // Pop
//{{{
void battle_pop(void* data)
{
	// Close
	thread_changegamephase(GAMEPHASE_LOADING);

	battledata* b = (battledata*)data;
	gamemod* gm = b->gm;

	// Return signifies if player survived battle
	if(*(gm->ammo)>0) // If alive
	{
		*(gm->money)+= 1000;
		*(gm->returnvalue) = 1;
	}
	else
	{
		*(gm->returnvalue) = 0;
	}

	free(gm);

	free(b);

	free(th_itd);
	th_itd = 0;

	// Delete Benchmarks
	debug_benchdelete("Battle");
	debug_benchdelete("Kinematic");
	debug_benchdelete("Dynamics");
	debug_benchdelete("Test1");
	debug_benchdelete("Test2");
	debug_benchdelete("Render");
	debug_benchdelete("Update");

}
//}}}

//}}}

// Battle Functions
//{{{

 // Effects
//{{{
void battle_effectadd(battledata* b, int type, intv x, intv y, intv z)
{
	neweffectlistnode* n = (neweffectlistnode*)malloc(sizeof(neweffectlistnode));

	n->type = type;
	n->px = x;
	n->py = y;
	n->pz = z;
	//xx
	n->next = 0;

	*(b->neweffectlisttailpointer) = n;
	b->neweffectlisttailpointer = &(n->next);
}
//}}}

 // Update ITD
//{{{
void battle_updateitd(battleitd* bitd, battledata* b)
{
	// NOTE Make sure to only modify the non render buffer buffer!!
	// Only the render thread is allowed to swap the buffers, and should
	//  only do this when the mutex is locked!
	// Make damn sure code Main thread doesn't touch render buffer data!!

	bitd->freshflag = 1;
	// Pointer to alternate buffer
	battlerenderbuffer* abrb = bitd->brb+(bitd->renderbufferid^1);
	abrb->starttime = thread_gettime();
	abrb->maintickcount = b->tickcount;
	int oc = 0; // object count; number of objects in render buffer

	// Camera data
	abrb->camx = ((float)(b->camx))/((float)UPM); // FLAGTE make int
	abrb->camy = ((float)(b->camy))/((float)UPM);
	abrb->camz = ((float)(b->camz))/((float)UPM);
	abrb->camlx = ((float)(b->camlx))/((float)UPM); // FLAGTE make int
	abrb->camly = ((float)(b->camly))/((float)UPM);
	abrb->camlz = ((float)(b->camlz))/((float)UPM);
	abrb->camtheta = b->camtheta;
	abrb->camphi = b->camphi;

	// HUD data
	{
		arsenal* ars = b->gm->ars;
		abrb->hudevel = ba_guntypes.escapevelocity[ars->guntype[b->equippedgun]];
	}

	// Add missles to render buffer
	{
		int i;
		missledata* md = &(b->md);
		int m = md->misslecount;
		intv vx, vy, vz;
		inta theta;
		// FLAG Make below loop into mutliple DOD friendly loops??
		// FLAGTE replace by passing angles instead of matrix
		for(i=0;i<m;i++)
		{
			abrb->romodelnum[oc] = ba_missletypes.model[md->type[i]];
			/*
			testmat.cell[0] = 1.0f;
			testmat.cell[1] = 0.0f;
			testmat.cell[2] = 0.0f;
			testmat.cell[3] = 0.0f;
			testmat.cell[4] = 1.0f;
			testmat.cell[5] = 0.0f;
			testmat.cell[6] = 0.0f;
			testmat.cell[7] = 0.0f;
			testmat.cell[8] = 1.0f;
			testmat.cell[9] = ((float)(md->px[i]))/((float)UPM);
			testmat.cell[10] = ((float)(md->py[i]))/((float)UPM);
			testmat.cell[11] = ((float)(md->pz[i]))/((float)UPM);
			*/
			abrb->ropx[oc] = md->px[i]-b->camx;
			abrb->ropy[oc] = md->py[i]-b->camy;
			abrb->ropz[oc] = md->pz[i]-b->camz;
			vx = md->vx[i];
			vy = md->vy[i];
			vz = md->vz[i];
			ATAN2(vy,vx,theta); // FLAGTE base off vel not pos!
			abrb->rotheta[oc] = theta;
			ATAN2(vz,SQR(VMV(vx,vx)+VMV(vy,vy)),abrb->rophi[oc]);


			//abrb->rosmatrix[oc] = testmat;
			oc++;
		}
	}

	// Add enemies to render buffer
	{
		int i;
		enemydata* ed = &(b->ed);
		int e = ed->enemycount;
		// FLAG Make below loop into mutliple DOD friendly loops??
		// FLAGTE replace by passing angles instead of matrix
		for(i=0;i<e;i++)
		{
			abrb->romodelnum[oc] = ba_enemytypes.model[ed->type[i]];
			abrb->ropx[oc] = ed->px[i] - b->camx;
			abrb->ropy[oc] = ed->py[i] - b->camy;
			abrb->ropz[oc] = ed->pz[i] - b->camz;
			abrb->rotheta[oc] = ed->theta[i];
			abrb->rophi[oc] = ed->phi[i];

			oc++;
		}

		abrb->renderobjectcount=oc;
	}

	
	// Update effect list
	{
		if(b->neweffectlisthead)
		{
			*(abrb->neweffectlisttailpointer) = b->neweffectlisthead;
			abrb->neweffectlisttailpointer = b->neweffectlisttailpointer;
		}
		b->neweffectlisthead = 0;
		b->neweffectlisttailpointer = &(b->neweffectlisthead);
	}

	/*
	abrb->renderobjectcount=4;
	// make matrices
	shortmatrix testmat;
	testmat.cell[0] = 1.0f;
	testmat.cell[1] = 0.0f;
	testmat.cell[2] = 0.0f;
	testmat.cell[3] = 0.0f;
	testmat.cell[4] = 1.0f;
	testmat.cell[5] = 0.0f;
	testmat.cell[6] = 0.0f;
	testmat.cell[7] = 0.0f;
	testmat.cell[8] = 1.0f;
	testmat.cell[9] = 0.0f;
	testmat.cell[10] = 0.0f;
	testmat.cell[11] = 0.0f;
	abrb->romodelnum[0] = 1;
	abrb->rosmatrix[0] = testmat;
	testmat.cell[9] = 2.0f;
	testmat.cell[10] = 2.0f;
	testmat.cell[11] = 0.0f;
	abrb->romodelnum[1] = 0;
	abrb->rosmatrix[1] = testmat;
	testmat.cell[9] = 2.0f;
	testmat.cell[10] = -2.0f;
	testmat.cell[11] = 0.0f;
	abrb->romodelnum[2] = 0;
	abrb->rosmatrix[2] = testmat;
	testmat.cell[9] = 2.0f;
	testmat.cell[10] = 2.0f;
	testmat.cell[11] = 2.0f;
	abrb->romodelnum[3] = 1;
	abrb->rosmatrix[3] = testmat;
	*/
}
//}}}

 // Kinematics
//{{{
intv DEMOtimer = -20*TPS;
void battle_kinematics(battledata* b)
{

	gamemod* gm = b->gm;
	inputdatabattle input;

	// Fetch input
	input = input_battle();

	// Player Copter
	//{{{
	{

		// Rotate cam (eventually fuck around with seperate cannon rot)
		b->camltheta = b->camtheta;
		b->camlphi = b->camphi;
		b->camtheta+=input.rotatex;
		b->camphi+=input.rotatey;

		// Adjust copter velocity
		//{{{
		{

			// FLAG eventually base on copter stats from game GAMEMOD
			intv taccel;	// acceleration
			intu tairres;	// air resistance
			{
				intv tmaxvel = 16*UPM;	// maximum copter velocity CHECK AGAINST MAX
				taccel = tmaxvel*3/2; // maxvel in 1.5 seconds
				tairres = VDV(tmaxvel,(tmaxvel+taccel/TPS));
			}

			// Apply acceleration to copter from thrusters
			{
				// Vector for desired thrust direction
				intv ix = 0;
				intv iy = 0;
				intv iz = 0;

				// Set value for input vector based on input variables
				if(input.forward)
				{
					int ct,st;
					ANGTOCS(b->camtheta,ct,st)
					ix+= ct;
					iy+= st;
				}
				if(input.back)
				{
					int ct,st;
					ANGTOCS(b->camtheta,ct,st)
					ix-= ct;
					iy-= st;
				}
				if(input.left)
				{
					int ct,st;
					ANGTOCS(b->camtheta,ct,st)
					ix-= st;
					iy+= ct;
				}
				if(input.right)
				{
					int ct,st;
					ANGTOCS(b->camtheta,ct,st)
					ix+= st;
					iy-= ct;
				}
				if(input.up) iz+= UPM;
				if(input.down) iz-= UPM;

				// Normalize input of mag over 1
				if( VMV(ix,ix)+VMV(iy,iy)+VMV(iz,iz) > UPM )
				{
					NORMALIZE(ix,iy,iz)
				}

				// Apply acceleration to copter velocity
				b->coptervx+= UMV(ix,taccel)/TPS;
				b->coptervy+= UMV(iy,taccel)/TPS;
				b->coptervz+= UMV(iz,taccel)/TPS;

			}

			// Apply Air resistance
			b->coptervx = UMV(tairres,b->coptervx);
			b->coptervy = UMV(tairres,b->coptervy);
			b->coptervz = UMV(tairres,b->coptervz);

			// Check that copter vel doesn't exceed max
			if(b->coptervx > MAXCOPTERVELOCITY)
			{
				b->coptervx = MAXCOPTERVELOCITY;
				debug_errormessage("Max copter velocity exceeded.\n");
			}
			if(-b->coptervx > MAXCOPTERVELOCITY)
			{
				b->coptervx = -MAXCOPTERVELOCITY;
				debug_errormessage("Max copter velocity exceeded.\n");
			}
			if(b->coptervy > MAXCOPTERVELOCITY)
			{
				b->coptervy = MAXCOPTERVELOCITY;
				debug_errormessage("Max copter velocity exceeded.\n");
			}
			if(-b->coptervy > MAXCOPTERVELOCITY)
			{
				b->coptervy = -MAXCOPTERVELOCITY;
				debug_errormessage("Max copter velocity exceeded.\n");
			}

		}
		//}}}

		// Apply copter velocity to position
		b->copterlpx = b->copterpx;
		b->copterlpy = b->copterpy;
		b->copterlpz = b->copterpz;
		b->copterpx+= b->coptervx/TPS;
		b->copterpy+= b->coptervy/TPS;
		b->copterpz+= b->coptervz/TPS;

		// Set camcoordinates to copter coordinates
		b->camlx = b->camx;
		b->camly = b->camy;
		b->camlz = b->camz;
		b->camx = b->copterpx;
		b->camy = b->copterpy;
		b->camz = b->copterpz;

		// Gun Code
		//{{{
		{
			// Temp vars
			arsenal* a = gm->ars;
			int guns = a->guncount;
			int eqgun;

			// Check for gun swap player input
			if(input.gunchange!=-1)
			{
				if(input.gunchange>=0&&input.gunchange<guns)
				{
					b->equippedgun = input.gunchange;
				}
			}
			eqgun = b->equippedgun;

			// Check for ammo swap player input
			//{{{
			if(input.ammochange)
			{
				int nav = input.ammochange;
				int dir = (nav>0)?1:-1;
				int boxcount = a->ammoboxcount;
				int box;
				int ammoclass = ba_guntypes.ammoclass[a->guntype[eqgun]];

				// just to be safe in case no matching ammo
				int safecount = nav*dir+1;
				int start = a->gunammobox[eqgun];

				box = start;
				if(boxcount>0)
				{
					while(nav)
					{
						int boxclass;
						// Cycle through boxes in dir direction
						box+= dir+boxcount;
						box%= boxcount;

						boxclass = ba_missletypes.ammoclass[a->ammoboxmissletype[box]];

						if(boxclass==ammoclass)
						{
							nav-= dir;
							if(!nav)
							{
								a->gunammobox[eqgun] = box;
								debug_message("New box: %d\n",box);// FLAG
							}
							if(box==start)
							{
			/* Checks to see if have looped through all ammo boxes more than
				ammochange times.  If so, must not have appropriate ammo. */
								safecount--;
								if(safecount<=0)
								{
		debug_errormessage("Error, no matching ammo boxes for this gun in arsenal.\n");
									nav = 0;
								}
							}
						}
					}
				}
				else
				{
		debug_errormessage("Error: no ammoboxes in arsenal.\n");
				}
			}
			//}}}

			// Run routines for every gun in arsenal
			int i;
			for(i=0;i<guns;i++)
			{

				if(i!=eqgun)
				{
					// Update gun phases
					a->gunphase[i]-= BATTLEPERIOD;
					if(a->gunphase[i]<0) a->gunphase[i]=0;
				}
				else
				{
					// Run routine of the current gun
					int gunroutine = ba_guntypes.routine[a->guntype[i]];
					(*ba_grfunctions[gunroutine])(b,&input,i);
// FLAGTE should checks be done before each shot to make sure that no indexes are out
//  of bounds, and that the linked ammotype from the ammobox works with the current gun?
// maybe just make the checks part of DEBUG compile?
				}
			}

		}
		//}}}

	}
	//}}}

	// Enemies
	//{{{
	{
		enemydata* ed = &(b->ed);

		/*
		if(input.testpress)
		{
			inta test = VMV(irand(),PI2)*4;
			inta test2 = VMV(irand(),PI2/2);
			battle_enemyadd(ed,((irand()&7)-4)*20*UPM,((irand()&7)-4)*20*UPM,0,test,test2,irand()*ba_maxenemytypes/UPM);
		}
		*/
		if(input.testpress)
		{
			DEMOtimer=0;
		}
		if(--DEMOtimer<=0)
		{
			//xx
			int type = irand()%10;
			if(type<6)
			{
				type = 0;
			}
			else if(type<9)
			{
				type = 2;
			}
			else
			{
				type = 1;
			}
			int angle = UMV(irand(),PI2)-PI2/2;
			int fangle = UMV(irand(),PI2)*4;
			int newx, newy;
			int distance = 100;
			ANGTOCS(angle,newx,newy);
			newx*= distance;
			newy*= distance;
			battle_enemyadd(ed,newx,newy,0,fangle,0,type);
			switch(type)
			{
				case 0:
					DEMOtimer+= TPS/2;
					break;
				case 2:
					DEMOtimer+= 2*TPS;
					break;
				case 1:
					DEMOtimer+= 4*TPS;
					break;
			}
		}

		// Move enemies based on enemy type routine
		{
			int i;
			int type;
			int e = ed->enemycount;
			for(i=0;i<e;i++)
			{

				ed->lpx[i] = ed->px[i];
				ed->lpy[i] = ed->py[i];
				ed->lpz[i] = ed->pz[i];
				type = ed->type[i];

				// Move
				//{{{
				switch(ba_enemytypes.enemyroutine[type])
				{
					case BA_ERTROOP:
					case BA_ERTANK:
					case BA_ERJEEP:
					{
						//check if copter in range
						// if so, stand and shoot
						// if not, turn towards base and walk

						intv dx, dy;
						inta dir;
						inta diff;
						intv vel = ba_enemytypes.vel[type];
						inta tvel = ba_enemytypes.absturnvel[type]/TPS +\
						VMV(ba_enemytypes.relturnvel[type],vel/TPS);

						// Apply velocity
						ed->px[i]+= UMV(ed->ct[i],vel/TPS);
						ed->py[i]+= UMV(ed->st[i],vel/TPS);

						// Turn to face middle
						ed->phi[i] = 0;
						dx = -ed->px[i];
						dy = -ed->py[i];
						ATAN2(dy,dx,dir);
						diff = dir-ed->theta[i];
						if(diff>0)
						{
							if(diff>tvel)
							{
								ed->theta[i]+= tvel;
							}
							else
							{
								ed->theta[i] = dir;
							}
						}
						else
						{
							if(diff<tvel)
							{
								ed->theta[i]-= tvel;
							}
							else
							{
								ed->theta[i] = dir;
							}
						}
					}
					break;

					case BA_ERMISSLE:
					{

						intv dx, dy, dz;
						inta dir;
						inta diff;
						intu ct = ed->ct[i];
						intu st = ed->st[i];
						intu cp = ed->cp[i];
						intu sp = ed->sp[i];
						intv vel = ba_enemytypes.vel[type];
						inta tvel = ba_enemytypes.absturnvel[type]/TPS;

						// Apply velocity
						ed->px[i]+= UMV(UMU(cp,ct),vel/TPS);
						ed->py[i]+= UMV(UMU(cp,st),vel/TPS);
						ed->pz[i]+= UMV(sp,vel/TPS);

						// turn to face player
						// FLAGTE eventually base on quat, then
						//  convert to euler
						dx = b->copterpx-ed->px[i];
						dy = b->copterpy-ed->py[i];
						dz = b->copterpz-ed->pz[i];
						ATAN2(dy,dx,dir);
						diff = dir-ed->theta[i];
						if(diff>0)
						{
							if(diff>tvel)
							{
								ed->theta[i]+= tvel;
							}
							else
							{
								ed->theta[i] = dir;
							}
						}
						else
						{
							if(diff<tvel)
							{
								ed->theta[i]-= tvel;
							}
							else
							{
								ed->theta[i] = dir;
							}
						}
						ATAN2(dz,SQR(VMV(dx,dx)+VMV(dy,dy)),dir);
						diff = dir-ed->phi[i];
						if(diff>0)
						{
							if(diff>tvel)
							{
								ed->phi[i]+= tvel;
							}
							else
							{
								ed->phi[i] = dir;
							}
						}
						else
						{
							if(diff<tvel)
							{
								ed->phi[i]-= tvel;
							}
							else
							{
								ed->phi[i] = dir;
							}
						}
					}
					break;
				}
				//}}}

				// Update trig values
				ANGTOCS(ed->theta[i],ed->ct[i],ed->st[i])
				ANGTOCS(ed->phi[i],ed->cp[i],ed->sp[i])

			}
		}

	}
	//}}}

	// Missles
	//{{{
	{

		int i;
		int g;
		missledata* md = &(b->md);
		int m = md->misslecount;

		// Apply velocity to position
		for(i=0;i<m;i++) md->lpx[i] = md->px[i];
		for(i=0;i<m;i++) md->lpy[i] = md->py[i];
		for(i=0;i<m;i++) md->lpz[i] = md->pz[i];
		for(i=0;i<m;i++) md->px[i]+= md->vx[i]/TPS;
		for(i=0;i<m;i++) md->py[i]+= md->vy[i]/TPS;
		for(i=0;i<m;i++) md->pz[i]+= md->vz[i]/TPS;

		// Apply gravity to velocity
		for(i=0;i<m;i++)
		{
			g = UMV(ba_missletypes.gravmod[md->type[i]],GRAVITY);
			md->vz[i]+= g/TPS;
		}

		// Check that velocity is within maximum
		for(i=0;i<m;i++)
		{
			intv rad = ba_ammoclasses.radius[ba_missletypes.ammoclass[md->type[i]]];
			if(md->vx[i]/TPS+rad > COLLISIONMISSLERADIUS)
			{
				debug_errormessage("Error: Max missle velocity exceeded.\n");
				md->life[i] = 0;
			}
			if(rad-md->vx[i]/TPS > COLLISIONMISSLERADIUS)
			{
				debug_errormessage("Error: Max missle velocity exceeded.\n");
				md->life[i] = 0;
			}
		}
		for(i=0;i<m;i++)
		{
			intv rad = ba_ammoclasses.radius[ba_missletypes.ammoclass[md->type[i]]];
			if(md->vy[i]/TPS+rad > COLLISIONMISSLERADIUS)
			{
				debug_errormessage("Error: Max missle velocity exceeded.\n");
				md->life[i] = 0;
			}
			if(rad-md->vy[i]/TPS > COLLISIONMISSLERADIUS)
			{
				debug_errormessage("Error: Max missle velocity exceeded.\n");
				md->life[i] = 0;
			}
		}

		// Decrement missle life
		for(i=0;i<m;i++)
		{
			md->life[i]-= BATTLEPERIOD;
		}

	}
	//}}}

}
//}}}

 // Dynamics
//{{{
void battle_dynamics(battledata* b)
{

	// Variables
	//{{{
	enemydata* ed = &(b->ed);
	missledata* md = &(b->md);
	//}}}

	// Copter ground
	// ..nothing yet, put in ground check when adding death conditions and enemy checks

	// Copter enemies
	// ...

	// Missle ground check
	//{{{
	{
		int i;
		int mc = md->misslecount;

		for(i=0;i<mc;i++)
		{
			if(md->pz[i]<=0)
			{
				md->life[i] = 0;
				switch(md->type[i])
				{
					case 0:
						battle_effectadd(b,2,md->px[i],md->py[i],md->pz[i]);
						break;
					case 1:
						//xx
						battle_effectadd(b,3,md->px[i],md->py[i],md->pz[i]);
						{
							int e;
							int rad = 7*UPM;
							int srad = VMV(rad,rad);
							int x = md->px[i];
							int y = md->py[i];
							int dx,dy,mag;
							int ec = ed->enemycount;
							for(e=0;e<ec;++e)
							{
								dx = ed->px[e] - x;
								dy = ed->py[e] - y;
								mag = VMV(dx,dx)+VMV(dy,dy);
								if(mag<srad)
								{
									mag = UPM - VDV(mag,srad);
									ed->health[e]-= 200*mag/UPM;
								}
							}
						}
						break;
				}
			}
		}
	}
	//}}}

	// Missle enemy collision check
	//{{{
	{

		debug_benchstart("Test2");
#if DEBUG_RAWCOLLISIONTEST
		int hitcount = 0;
		int newhitcount = 0;

		// Raw ellipsoid/sphere test
		//{{{
		{

			// FLAGTE do brute force vs efficient and check that consistent
			//  maybe some collision data structure that can be easily checked
			int mc = md->misslecount;
			int mip[mc]; // missle in play
			{
				int i;
				for(i=0;i<mc;i++)
				{
					mip[i]=1;
				}
			}

			int e;
			int ec = ed->enemycount;

			// Cycle through enemies, and check each missle against
			for(e=0;e<ec;e++)
			{

				// Enemy data
				intv esx = ed->lpx[e];
				intv esy = ed->lpy[e];
				intv esz = ed->lpz[e];
				intv eex = ed->px[e];
				intv eey = ed->py[e];
				intv eez = ed->pz[e];
				int etype = ed->type[e];
				intv eox = ba_enemytypes.ox[etype];
				intv eoy = ba_enemytypes.oy[etype];
				intv eoz = ba_enemytypes.oz[etype];
				intv erx = ba_enemytypes.rx[etype];
				intv ery = ba_enemytypes.ry[etype];
				intv erz = ba_enemytypes.rz[etype];
				intu m0,m1,m2,m3,m4,m5,m6,m8; // m7 = 0
				{
					// coefficients for reverse rotation matrix
					intu ct = ed->ct[e];
					intu st = ed->st[e];
					intu cp = ed->cp[e];
					intu sp = ed->sp[e];
					m0 = UMU(ct,cp);
					m3 = UMU(st,cp);
					m6 = sp;
					m1 = -st;
					m4 = ct;
					//m7 = 0
					m2 = -UMU(ct,sp);
					m5 = -UMU(st,sp);
					m8 = cp;
				}

				int m;
				int mc = md->misslecount;
				for(m=0;m<mc;m++)
				{
					if(mip[m]){
					int mtype = md->type[m];

					// Get relative position
					intv dsx = md->lpx[m] - esx;
					intv dsy = md->lpy[m] - esy;
					intv dsz = md->lpz[m] - esz;
					intv dex = md->px[m] - eex;
					intv dey = md->py[m] - eey;
					intv dez = md->pz[m] - eez;

					// Get relative rotation
					intv rsx = UMV(m0,dsx) + UMV(m3,dsy) + UMV(m6,dsz);
					intv rsy = UMV(m1,dsx) + UMV(m4,dsy);
					intv rsz = UMV(m2,dsx) + UMV(m5,dsy) + UMV(m8,dsz);
					intv rex = UMV(m0,dex) + UMV(m3,dey) + UMV(m6,dez);
					intv rey = UMV(m1,dex) + UMV(m4,dey);
					intv rez = UMV(m2,dex) + UMV(m5,dey) + UMV(m8,dez);

					// Adjust for collision ellipsoid offset
					rsx-= eox;
					rsy-= eoy;
					rsz-= eoz;
					rex-= eox;
					rey-= eoy;
					rez-= eoz;

					// Scale so enemy zone is unit sphere
					intv mr = ba_ammoclasses.radius[ba_missletypes.ammoclass[mtype]];
					rsx = VDV(rsx,erx+mr);
					rsy = VDV(rsy,ery+mr);
					rsz = VDV(rsz,erz+mr);
					rex = VDV(rex,erx+mr);
					rey = VDV(rey,ery+mr);
					rez = VDV(rez,erz+mr);

					// Coordinates of  (e-s)  vector (d)
					intv dx = rex - rsx;
					intv dy = rey - rsy;
					intv dz = rez - rsz;

					// (d).(-s)   dot product
					intv dps = VMV(dx,-rsx)+VMV(dy,-rsy)+VMV(dz,-rsz);

					// (-d).(-e)  dot product
					intv dpe = VMV(-dx,-rex)+VMV(-dy,-rey)+VMV(-dz,-rez);

					// (d).(d)    dot product
					intv dpd = VMV(dx,dx)+VMV(dy,dy)+VMV(dz,dz);
					// approx of (|d|+1)^2
					intv dsa = dpd*3/2+UPM*2;

					// Test for normal cylinder intersection
					if( dps<dsa && dpe<=dsa )
					{

						// Coords of  (d)X(-s)  cross product (c)
						intv cx = VMV(dy,-rsz) - VMV(dz,-rsy);
						intv cy = VMV(dz,-rsx) - VMV(dx,-rsz);
						intv cz = VMV(dx,-rsy) - VMV(dy,-rsx);

						intv cpc = VMV(cx,cx)+VMV(cy,cy)+VMV(cz,cz);

						// Test if intersection within unit sphere
						// if  (c).(c) < (d).(d)
						if(cpc < dpd)
						{
							// Collision time: 0 < ct <= 1
							intv ct = VDV(dps,dpd) - 
								SQR(VDV(UPM-VDV(cpc,dpd),dpd));
							
			//debug_message("E: %d  M: %d  CT: %f\n",e,m,((float)ct)/UPM);


							if( ct> 0 && ct<=UPM )
							{
								//debug_message("HIT\n");
								hitcount++;
								mip[m] = 0;
							}
						}
					}
					}// end mipcheck

				}
			}
		}
		//}}}
#endif
		debug_benchstop("Test2","");

		debug_benchstart("Test1");

		/*
		// Bounding box test
		//{{{
		{

			// Bounding box array for enemies
			typedef struct{intv lx,hx,ly,hy,lz,hz;} bbox;
			bbox ebb[MAXENEMIES];
			// FLAGTE add grid cells... maybe have buckets simply be lists,
			//  and MAXENEMIES size array, each associated with an enemy,
			//  and serve as the "next" pointer in a list
			
			// Generate bounding boxes for enemies
			//{{{
			{
				int i;
				intv tpx,tpy,tpz;
				intv tvx,tvy,tvz;
				intv tbr;
				int type;
				intu ct,st;
				intu cp,sp;
				intv vel;
				int e = ed->enemycount;

				for(i=0;i<e;i++)
				{
					tpx = ed->px[i];
					tpy = ed->py[i];
					tpz = ed->pz[i];
					type = ed->type[i];
					vel = ba_enemytypes.vel[type]/TPS;
					ct = ed->ct[i];
					st = ed->st[i];
					cp = ed->cp[i];
					sp = ed->sp[i];
					tvx = UMV(UMU(cp,ct),vel);
					tvy = UMV(UMU(cp,st),vel);
					tvz = UMV(sp,vel);
					tbr = ba_enemytypes.bbr[type];

					if(tvx>0)
					{
						ebb[i].lx = tpx-tbr;
						ebb[i].hx = tpx+tvx+tbr;
					}
					else
					{
						ebb[i].lx = tpx+tvx-tbr;
						ebb[i].hx = tpx+tbr;
					}

					if(tvy>0)
					{
						ebb[i].ly = tpy-tbr;
						ebb[i].hy = tpy+tvy+tbr;
					}
					else
					{
						ebb[i].ly = tpy+tvy-tbr;
						ebb[i].hy = tpy+tbr;
					}

					if(tvz>0)
					{
						ebb[i].lz = tpz-tbr;
						ebb[i].hz = tpz+tvz+tbr;
					}
					else
					{
						ebb[i].lz = tpz+tvz-tbr;
						ebb[i].hz = tpz+tbr;
					}
				}
			}
			//}}}

			// Test for missle/enemy box overlap
			//{{{
			{
				int i;
				int m = md->misslecount;

				for(i=0;i<m;i++)
				{

					// Bounding box for missle
					bbox mbb;

					// Set missle bounding box
					//{{{
					{

						intv tpx = md->px[i];
						intv tpy = md->py[i];
						intv tpz = md->pz[i];
						intv tvx = md->vx[i]/TPS;
						intv tvy = md->vy[i]/TPS;
						intv tvz = md->vz[i]/TPS;
						int type = ed->type[i];
						intv tbr = ba_missletypes.radius[type];

						if(tvx>0)
						{
							mbb.lx = tpx-tbr;
							mbb.hx = tpx+tvx+tbr;
						}
						else
						{
							mbb.lx = tpx+tvx-tbr;
							mbb.hx = tpx+tbr;
						}

						if(tvy>0)
						{
							mbb.ly = tpy-tbr;
							mbb.hy = tpy+tvy+tbr;
						}
						else
						{
							mbb.ly = tpy+tvy-tbr;
							mbb.hy = tpy+tbr;
						}

						if(tvz>0)
						{
							mbb.lz = tpz-tbr;
							mbb.hz = tpz+tvz+tbr;
						}
						else
						{
							mbb.lz = tpz+tvz-tbr;
							mbb.hz = tpz+tbr;
						}

					}
					//}}}

					int j;
					int e = ed->enemycount;
					for(j=0;j<e;j++)
					{
						if(mbb.lx < ebb[j].hx &&
						ebb[j].lx < mbb.hx &&
						mbb.ly < ebb[j].hy &&
						ebb[j].ly < mbb.hy &&
						mbb.lz < ebb[j].hz &&
						ebb[j].lz < mbb.hz)
						{
							// Add to list
						}
					}

				}

			}
			//}}}

		}
		//}}}
		*/
		
		// Note: if other entities are eventually allowed to spawn missles,
		//  have them have their own collisions grids, and assign each missle
		//  a value corresponding to their source.
		//  Then for each enemy, check against the grids of all missle spawning entities,
		//  a number which should be limited to like a couple dozen.

		// Nodes for hash bucket lists
		//  Each grid cell has a head node which points to the head missle in
		//  the gridcell bucket list, and the list nodes contain the index of the
		//  next missle in the list (a value of -1 represents the end of the list)
		int i;
		int mc = md->misslecount;
		const int bucketcount = COLLISIONGRIDWIDTHINCELLS*COLLISIONGRIDWIDTHINCELLS;
		int gbheadnode[bucketcount];	// Acts as the head of a linked list for a bucket
		int gblistnode[mc];		// Acts as the "next" field of a linked list node

		// Initiliaze all head nodes so that they're empty/null
		for(i=0;i<bucketcount;i++)
		{
			gbheadnode[i] = -1;
		}

		// Add missles to appropriate grid bucket lists
		//{{{
		for(i=0;i<mc;i++)
		{
			if(md->pz[i] > 3*UPM) goto missle_hash_end;
			// Missle position relative to copter
			intv dx = md->px[i] - b->copterpx;
			intv dy = md->py[i] - b->copterpy;

			// Check that missle is within maxrange; delete if without
			{
				int dts = (dx/UPM)*(dx/UPM)+(dy/UPM)*(dy/UPM);
				int mmrs = (MAXMISSLERANGE/UPM)*(MAXMISSLERANGE/UPM);

				if( dts > mmrs )
				{
					debug_message("Missle %d has exceeded maximum range.\n",i);
					md->life[i] = 0;
					goto missle_hash_end;
				}
			}

			// Get index of grid cell that corresponds to missle position
			int gcindex;
			{
				int gx = (dx + COLLISIONGRIDWIDTH/2) >> COLLISIONCELLWIDTHSHIFT;
				int gy = (dy + COLLISIONGRIDWIDTH/2) >> COLLISIONCELLWIDTHSHIFT;
				gcindex = gx + gy*COLLISIONGRIDWIDTHINCELLS;
			}

			// Check that index value is valid
			if( gcindex<0 || gcindex>=bucketcount )
			{
				debug_message("Error: Missle %d has somehow produced an invald collision grid index value.\n",i);
				md->life[i] = 0;
				goto missle_hash_end;
			}

			// Add missle to grid bucket list of appropriate grid cell
			//  Set missle's next-pointer to previous head of list
			gblistnode[i] = gbheadnode[gcindex];
			//  Set missle index to the head of the list
			gbheadnode[gcindex] = i;

			// Jump point for early termination
			missle_hash_end: ;

		}
		//}}}

		// Check enemies against missles in appropriate bucket
		//{{{
		{

			int e;
			int ec = ed->enemycount;

			// Cycle through enemies, and check each missle against
			for(e=0;e<ec;e++)
			{

				// x/y coords
				intv eex = ed->px[e];
				intv eey = ed->py[e];

				// Check that enemy is within grid
				if(eex - b->copterpx >= COLLISIONGRIDWIDTH/2 &&
				b->copterpx - eex >= COLLISIONGRIDWIDTH/2 &&
				eey - b->copterpy >= COLLISIONGRIDWIDTH/2 &&
				b->copterpy - eey >= COLLISIONGRIDWIDTH/2)
				{
					goto enemy_check_end; //FLAGTE replace with normal block
				}

				// Load enemy data
				//{{{
				intv eez = ed->pz[e];
				intv esx = ed->lpx[e];
				intv esy = ed->lpy[e];
				intv esz = ed->lpz[e];
				int etype = ed->type[e];
				intv eox = ba_enemytypes.ox[etype];
				intv eoy = ba_enemytypes.oy[etype];
				intv eoz = ba_enemytypes.oz[etype];
				intv erx = ba_enemytypes.rx[etype];
				intv ery = ba_enemytypes.ry[etype];
				intv erz = ba_enemytypes.rz[etype];
				intv ebbr = ba_enemytypes.bbr[etype];
				intu m0,m1,m2,m3,m4,m5,m6,m8; // m7 = 0
				{
					// coefficients for reverse rotation matrix
					intu ct = ed->ct[e];
					intu st = ed->st[e];
					intu cp = ed->cp[e];
					intu sp = ed->sp[e];
					m0 = UMU(ct,cp);
					m3 = UMU(st,cp);
					m6 = sp;
					m1 = -st;
					m4 = ct;
					//m7 = 0
					m2 = -UMU(ct,sp);
					m5 = -UMU(st,sp);
					m8 = cp;
				}
				//}}}

				// Determine grid cell rectangle to check
				//{{{
				int gcixl, gcixh, gciyl, gciyh; // rect of gridcells to check
				{

					if( eex > esx )
					{
						gcixl = esx-ebbr-COLLISIONMISSLERADIUS-b->copterpx;
						gcixl+= COLLISIONGRIDWIDTH/2;
						gcixl>>=COLLISIONCELLWIDTHSHIFT;
						gcixh = eex+ebbr+COLLISIONMISSLERADIUS-b->copterpx;
						gcixh+= COLLISIONGRIDWIDTH/2;
						gcixh>>=COLLISIONCELLWIDTHSHIFT;
					}
					else
					{
						gcixl = eex-ebbr-COLLISIONMISSLERADIUS-b->copterpx;
						gcixl+= COLLISIONGRIDWIDTH/2;
						gcixl>>=COLLISIONCELLWIDTHSHIFT;
						gcixh = esx+ebbr+COLLISIONMISSLERADIUS-b->copterpx;
						gcixh+= COLLISIONGRIDWIDTH/2;
						gcixh>>=COLLISIONCELLWIDTHSHIFT;
					}
					if( eey > esy )
					{
						gciyl = esy-ebbr-COLLISIONMISSLERADIUS-b->copterpy;
						gciyl+= COLLISIONGRIDWIDTH/2;
						gciyl>>=COLLISIONCELLWIDTHSHIFT;
						gciyh = eey+ebbr+COLLISIONMISSLERADIUS-b->copterpy;
						gciyh+= COLLISIONGRIDWIDTH/2;
						gciyh>>=COLLISIONCELLWIDTHSHIFT;
					}
					else
					{
						gciyl = eey-ebbr-COLLISIONMISSLERADIUS-b->copterpy;
						gciyl+= COLLISIONGRIDWIDTH/2;
						gciyl>>=COLLISIONCELLWIDTHSHIFT;
						gciyh = esy+ebbr+COLLISIONMISSLERADIUS-b->copterpy;
						gciyh+= COLLISIONGRIDWIDTH/2;
						gciyh>>=COLLISIONCELLWIDTHSHIFT;
					}

					// Check that within bounds
					if(gcixl < 0) gcixl = 0;
					if(gcixh >= COLLISIONGRIDWIDTHINCELLS)
						gcixh = COLLISIONGRIDWIDTHINCELLS-1;
					if(gciyl < 0) gciyl = 0;
					if(gciyh >= COLLISIONGRIDWIDTHINCELLS)
						gciyh = COLLISIONGRIDWIDTHINCELLS-1;
				}
				//}}}

				// Check against all missles in grid cell rectangle
				int ix,iy;
				for(iy=gciyl;iy<=gciyh;iy++)
				{
				for(ix=gcixl;ix<=gcixh;ix++)
				{
				int gcindex = ix + iy*COLLISIONGRIDWIDTHINCELLS;
				if( gcindex<0 || gcindex>=bucketcount )
				{
					debug_message("Error: Enemy %d has somehow produced an invald collision grid index value.\n",i);
					gcindex = 0;
				}
				int m = gbheadnode[gcindex];
#if DEBUG_RAWCOLLISIONTEST
				int* mp = &(gbheadnode[gcindex]);
				int keepmp;
#endif
				while(m>=0)
				{
#if DEBUG_RAWCOLLISIONTEST
					keepmp = 0;
#endif
					// FLAG this doesn't check bounding box or sphere first?
					//  ...maybe overkill

					int mtype = md->type[m];

					// Get relative position
					intv dsx = md->lpx[m] - esx;
					intv dsy = md->lpy[m] - esy;
					intv dsz = md->lpz[m] - esz;
					intv dex = md->px[m] - eex;
					intv dey = md->py[m] - eey;
					intv dez = md->pz[m] - eez;

					// Get relative rotation
					intv rsx = UMV(m0,dsx) + UMV(m3,dsy) + UMV(m6,dsz);
					intv rsy = UMV(m1,dsx) + UMV(m4,dsy);
					intv rsz = UMV(m2,dsx) + UMV(m5,dsy) + UMV(m8,dsz);
					intv rex = UMV(m0,dex) + UMV(m3,dey) + UMV(m6,dez);
					intv rey = UMV(m1,dex) + UMV(m4,dey);
					intv rez = UMV(m2,dex) + UMV(m5,dey) + UMV(m8,dez);

					// Adjust for collision ellipsoid offset
					rsx-= eox;
					rsy-= eoy;
					rsz-= eoz;
					rex-= eox;
					rey-= eoy;
					rez-= eoz;

					// Scale so enemy zone is unit sphere
					intv mr = ba_ammoclasses.radius[ba_missletypes.ammoclass[mtype]];
					// Vector (s)
					rsx = VDV(rsx,erx+mr);
					rsy = VDV(rsy,ery+mr);
					rsz = VDV(rsz,erz+mr);
					// Vector (e)
					rex = VDV(rex,erx+mr);
					rey = VDV(rey,ery+mr);
					rez = VDV(rez,erz+mr);

					// Coordinates of (e-s):  vector (d)
					intv dx = rex - rsx;
					intv dy = rey - rsy;
					intv dz = rez - rsz;

					// (d).(-s)   dot product
					intv dps = VMV(dx,-rsx)+VMV(dy,-rsy)+VMV(dz,-rsz);

					// (-d).(-e)  dot product
					intv dpe = VMV(-dx,-rex)+VMV(-dy,-rey)+VMV(-dz,-rez);

					// (d).(d)    dot product
					intv dpd = VMV(dx,dx)+VMV(dy,dy)+VMV(dz,dz);
					// approx of (|d|+1)^2
					intv dsa = dpd*3/2+UPM*2;

					// FLAGTE change below branch so that
					//  the comparison results are saved;
					//  then process the results in a loop after this,
					//  to avoid branching within this algo
					//  and make more SIMD friendly
					// Test for normal cylinder intersection
					if( dps<dsa && dpe<=dsa )
					{

						// Coords of  (d)X(-s)  cross product (c)
						intv cx = VMV(dy,-rsz) - VMV(dz,-rsy);
						intv cy = VMV(dz,-rsx) - VMV(dx,-rsz);
						intv cz = VMV(dx,-rsy) - VMV(dy,-rsx);

						intv cpc = VMV(cx,cx)+VMV(cy,cy)+VMV(cz,cz);

						// Test if intersection within unit sphere
						// if  (c).(c) < (d).(d)
						if(cpc < dpd)
						{
							// Collision time: 0 < ct <= 1
							intv ct = VDV(dps,dpd) - 
								SQR(VDV(UPM-VDV(cpc,dpd),dpd));
							
			//debug_message("E: %d  M: %d  CT: %f\n",e,m,((float)ct)/UPM);


							if( ct> 0 && ct<=UPM )
							{

			//xx
			// DEMO temp code herp derp

			// move missle to where it was at time of impact
			md->px[m] = VMV(ct,md->px[m]-md->lpx[m]) + md->lpx[m];
			md->py[m] = VMV(ct,md->py[m]-md->lpy[m]) + md->lpy[m];
			md->pz[m] = VMV(ct,md->pz[m]-md->lpz[m]) + md->lpz[m];
			
			// tank deflects bullets
			if(md->type[m] == 0)
			{
				if(ed->type[e] == 1)
				{
					// Bounce missles off normal:
					// vel-= 2*(vel.norm)*norm
					// Normal coordinates, unrotated
					intv nx = rsx+VMV(ct,rex-rsx);
					intv ny = rsy+VMV(ct,rey-rsy);
					intv nz = rsz+VMV(ct,rez-rsz);
					nx = VDV(nx,erx);
					ny = VDV(ny,ery);
					nz = VDV(nz,erz);
					NORMALIZE(nx,ny,nz);
					intu ct = ed->ct[e];
					intu st = ed->st[e];
					intu cp = ed->cp[e];
					intu sp = ed->sp[e];
					// Rotated normal coordinates
					//  (back to relative to ground)
					intu nrx = 
						UMU(nx,UMU(ct,cp))+
						UMU(-ny,st)+
						UMU(-nz,UMU(cp,sp));
					intu nry =
						UMU(nx,UMU(st,cp))+
						UMU(ny,ct)+
						UMU(-nz,UMU(st,sp));
					intu nrz =
						UMU(nx,sp)+
						UMU(nz,cp);
					intv dotmag =
						UMV(nrx,md->vx[m])+
						UMV(nry,md->vy[m])+
						UMV(nrz,md->vz[m]);
					md->vx[m]-= 2*UMV(nrx,dotmag);
					md->vy[m]-= 2*UMV(nry,dotmag);
					md->vz[m]-= 2*UMV(nrz,dotmag);

					// Dampen bounced velocity
					md->vx[m]/= 8;
					md->vy[m]/= 8;
					md->vz[m]/= 8;
				}
				else
				{
					md->life[m] = 0;
					--(ed->health[e]);
					battle_effectadd(b,4,md->px[m],md->py[m],md->pz[m]);
				}
			}
			if(md->type[m] == 1)
			{
				// spawn explosion
				//xx
				md->pz[m] = -1;
				ed->health[e]-= 100;
				/*
				battle_effectadd(b,3,
						ed->px[e]+irand()*3,
						ed->py[e]+irand()*3,
						ed->pz[e]);
						*/
			}

			// Add effect
			/*
			battle_effectadd(b,2,md->px[m],
				md->py[m],
				md->pz[m]);
			*/

			// Hit detected
			//md->life[m] = 0;
			//ed->health[e] = 0;
			// add VSQ vsquared and MAG vec magnitude?

#if DEBUG_RAWCOLLISIONTEST
	newhitcount++;
	*mp = gblistnode[m];
	keepmp = 1;
#endif
							}
						}
					}

					// Set m to next value in list
					m = gblistnode[m];
#if DEBUG_RAWCOLLISIONTEST
					if(!keepmp)
					{
						mp = &(gblistnode[m]);
					}
#endif

				}
				}}

				// Jump point for early termination
				enemy_check_end: ;

			}
		}
		//}}}

		debug_benchstop("Test1","");

#if DEBUG_RAWCOLLISIONCHECK
		if(hitcount!=newhitcount)
		{
			debug_message("COLLISION NOT DETECTED: brute-%d hash-%d\n",hitcount,newhitcount);
		}
#endif
	}
	//}}}
	
	// Check for missle and enemy deletion
	//{{{
	{
		int i;

		// Enemy health check
		for(i=ed->enemycount-1;i>=0;i--)
		{
			// Goes in reverse order so that data swapping doesn't cause
			//  an enemy check to be missed
			if(ed->health[i]<=0)
			{
				if(ed->type[i] == BA_ERTROOP)
					battle_effectadd(b,5,ed->px[i],ed->py[i],ed->pz[i]);
				else
					battle_effectadd(b,1,ed->px[i],ed->py[i],ed->pz[i]);
				battle_enemydelete(ed, i);
			}
		}

		// Missle life check
		for(i=md->misslecount-1;i>=0;i--)
		{
			// Goes in reverse order so that data swapping doesn't cause
			//  a missle check to be missed
			if(md->life[i]<=0)
			{
				battle_missledelete(md, i);
			}
		}
	}
	//}}}

}
//}}}

 // Enemy Functions
//{{{

void battle_enemyadd(enemydata* ed, intv px, intv py, intv pz, inta theta, inta phi, int type)
{
	// Fills in top of buffer with new enemy data and increments enemy count

	int e = ed->enemycount;

	if(e>=MAXENEMIES)
	{
		debug_errormessage("Enemy Spawn Failure: Maxenemies reached.\n");
		return;
	}
	/* FLAGTE alter for enemy types and uncomment
	if(type>=ba_maxmissletypes || type<0)
	{
		debug_errormessage("Missle Spawn Failure: Invalid missle type passed.\n");
		return;
	}
	*/

	ed->px[e] = px;
	ed->py[e] = py;
	ed->pz[e] = pz;
	ed->lpx[e] = px;
	ed->lpy[e] = py;
	ed->lpz[e] = pz;
	ed->theta[e] = theta;
	ANGTOCS(theta,ed->ct[e],ed->st[e]);
	ed->phi[e] = phi;
	ANGTOCS(phi,ed->cp[e],ed->sp[e]);
	ed->health[e] = ba_enemytypes.maxhealth[type];
	ed->type[e] = type;

	ed->enemycount++;
}

void battle_enemydelete(enemydata* ed, int enemy)
{
	// Swaps out deleted enemy data with top enemy data and decrements enemy count

	// FLAG when iterating through a list this function must be called in reverse order
	//  otherwise could swap top missle to middle and then go to delete an already
	//  deleted enemy 
	int t = ed->enemycount-1;

	if( enemy>t || enemy<0)
	{
		debug_errormessage("Enemy Deletion Failure: Invalid index (%d) passed.\n",enemy);
		return;
	}

	ed->px[enemy] = ed->px[t];
	ed->py[enemy] = ed->py[t];
	ed->pz[enemy] = ed->pz[t];
	ed->lpx[enemy] = ed->lpx[t];
	ed->lpy[enemy] = ed->lpy[t];
	ed->lpz[enemy] = ed->lpz[t];
	ed->theta[enemy] = ed->theta[t];
	ed->phi[enemy] = ed->phi[t];
	ed->ct[enemy] = ed->ct[t];
	ed->st[enemy] = ed->st[t];
	ed->cp[enemy] = ed->cp[t];
	ed->sp[enemy] = ed->sp[t];
	ed->health[enemy] = ed->health[t];
	ed->type[enemy] = ed->type[t];

	ed->enemycount--;
}

//}}}

 // Missle Functions
//{{{

void battle_missleadd(missledata* md, intv px, intv py, intv pz, intv vx, intv vy, intv vz, int type, intv life)
{
	// Fills in top of buffer with new missle data and increments missle count

	int m = md->misslecount;

	if(m>=MAXMISSLES)
	{
		debug_errormessage("Missle Spawn Failure: Maxmissles reached.\n");
		return;
	}
	if(type>=ba_maxmissletypes || type<0)
	{
		debug_errormessage("Missle Spawn Failure: Invalid missle type passed.\n");
		return;
	}

	md->px[m] = px;
	md->py[m] = py;
	md->pz[m] = pz;
	md->vx[m] = vx;
	md->vy[m] = vy;
	md->vz[m] = vz;
	md->lpx[m] = px;
	md->lpy[m] = py;
	md->lpz[m] = pz;
	md->type[m] = type;
	md->life[m] = life;

	md->misslecount++;
}

void battle_missledelete(missledata* md, int missle)
{
	// Swaps out deleted missle data with top missle data and decrements missle count

	// FLAG when iterating through a list this function must be called in reverse order
	//  otherwise could swap top missle to middle and then go to delete an already
	//  deleted missle
	int t = md->misslecount-1;

	if( missle>t || missle<0)
	{
		debug_errormessage("Missle Deletion Failure: Invalid index (%d) passed.\n",missle);
		return;
	}

	md->px[missle] = md->px[t];
	md->py[missle] = md->py[t];
	md->pz[missle] = md->pz[t];
	md->vx[missle] = md->vx[t];
	md->vy[missle] = md->vy[t];
	md->vz[missle] = md->vz[t];
	md->lpx[missle] = md->lpx[t];
	md->lpy[missle] = md->lpy[t];
	md->lpz[missle] = md->lpz[t];
	md->type[missle] = md->type[t];
	md->life[missle] = md->life[t];

	md->misslecount--;
}

//}}}

 // Shoot Missle
//{{{
void battle_shootmissle(battledata* b, inputdatabattle* input, int guntype, int missletype, intv time)
{

	// Aim vector data
	intu aimx, aimy, aimz;
	intv evel = ba_guntypes.escapevelocity[guntype];
	intv px,py,pz;
	intv vx,vy,vz;
	intv ox,oy,oz; // position offset for bullet spawn, center of rotation for cannon
	intv rox,roy,roz; // oxoyoz after rotation
	// FLAG have oxoyoz eventually be based on either global vars or guntype properties
	intu timemod = UPM-VDV(time,BATTLEPERIOD);

	// orientation: x forward, y left, z up
	ox = 0;
	oy = -UPM*1/4;
	oz = -UPM*3/4;

	// Generate unit vector values for aim based on input.aim
	{//{{{

		intv tx, ty, tz;
		intu ct,st;
		intu cp,sp;
		intv stray;
		intv strayx, strayy, strayz;
		inta theta, phi;

		// At angle 0,0 camera z is facing down x axis
		tx = input->aimz;
		ty = -input->aimlx-UMV(timemod,(input->aimx-input->aimlx));
		tz = input->aimly+UMV(timemod,(input->aimy-input->aimly));

		NORMALIZE(tx,ty,tz) // Now t vars are <=1.0, can treat as intu

		// Apply inaccuracy
		// FLAG sloppy, do better if have time
		stray = ba_guntypes.accuracyconetangent[guntype];
		if(stray)
		{
			strayx=strayy=strayz=0;
			while(strayx+strayz+strayz==0) // to avoid 0 divide
			{
				strayx = UMV(2*irand()-UPM,stray);
				strayy = UMV(2*irand()-UPM,stray);
				strayz = UMV(2*irand()-UPM,stray);
			}
			NORMALIZE(strayx,strayy,strayz);
			strayx = UMU(strayx,stray);
			strayy = UMU(strayy,stray);
			strayz = UMU(strayz,stray);
			// now have a somewhat random sphere of radius stray to add to aim vector
			tx+=strayx;
			ty+=strayy;
			tz+=strayz;
			NORMALIZE(tx,ty,tz)
		}

		// Get cos and sin for theta and phi
		theta = b->camltheta+UMV(timemod,(b->camtheta-b->camltheta));
		phi = b->camlphi+UMV(timemod,(b->camphi-b->camlphi));
		ANGTOCS(theta,ct,st)
		ANGTOCS(phi,cp,sp)

		// Multiply aim vector by camera rotation matrix
		aimx = UMU(tx,UMU(ct,cp));
		aimx+= UMU(ty,-st);
		aimx+= UMU(tz,-UMU(ct,sp));
		aimy = UMU(tx,UMU(st,cp));
		aimy+= UMU(ty,ct);
		aimy+= UMU(tz,-UMU(st,sp));
		aimz = UMU(tx,sp);
		//  += 0*ty
		aimz+= UMU(tz,cp);

		// Adjust aim for copter velocity
		if(1) // change to a check for a relative-aiming parameter set by player
		{
			intv tvx = -b->coptervx;
			intv tvy = -b->coptervy;
			intv tvz = -b->coptervz;
			intv adv = UMV(aimx,tvx)+UMV(aimy,tvy)+UMV(aimz,tvz);
			intv px = tvx - UMV(aimx,adv);
			intv py = tvy - UMV(aimy,adv);
			intv pz = tvz - UMV(aimz,adv);
			intv ada = VMV(evel,evel) - VMV(px,px)+VMV(py,py)+VMV(pz,pz);
			if(ada>0 && evel>0)
			{
				intv ine = VDV(UPM,evel);
				intv maga = SQR(ada);
				intv ax = UMV(aimx,maga);
				intv ay = UMV(aimy,maga);
				intv az = UMV(aimz,maga);
				aimx = VMV(ine,ax) + VMV(ine,px);
				aimy = VMV(ine,ay) + VMV(ine,py);
				aimz = VMV(ine,az) + VMV(ine,pz);
				NORMALIZE(aimx,aimy,aimz);
			}
		}

		// Rotate offset vector by cam rotation matrix
		// FLAGTE doesn't take aim into account, just cam rotation
		rox = UMV(UMU(ct,cp),ox);
		rox+= UMV(-st,oy);
		rox+= UMV(-UMU(ct,sp),oz);

		roy = UMV(UMU(st,cp),ox);
		roy+= UMV(ct,oy);
		roy+= UMV(-UMU(st,sp),oz);

		roz = UMV(sp,ox);
		// += 0*oy
		roz+= UMV(cp,oz);


	}//}}}

	vx = UMV(aimx,evel)+b->coptervx;
	vy = UMV(aimy,evel)+b->coptervy;
	vz = UMV(aimz,evel)+b->coptervz;
	px = b->copterlpx+UMV(timemod,b->copterpx-b->copterlpx) + rox + VMV(vx,time);
	py = b->copterlpy+UMV(timemod,b->copterpy-b->copterlpy) + roy + VMV(vy,time);
	pz = b->copterlpz+UMV(timemod,b->copterpz-b->copterlpz) + roz + VMV(vz,time);
	//px = b->copterpx + rox + VMV(vx,time); // FLAG DELETE ME
	//py = b->copterpy + roy + VMV(vy,time);
	//pz = b->copterpz + roz + VMV(vz,time);

	battle_missleadd( &(b->md),
			px,py,pz,
			vx,vy,vz,
			missletype,
			ba_guntypes.misslelife[guntype]);
}

/* saving in case want two versions, with and without time
void battle_shootmissle(battledata* b, inputdatabattle* input, int guntype, int missletype)
{

	// Aim vector data
	intu aimx, aimy, aimz;
	intv evel = ba_guntypes.escapevelocity[guntype];
	intv px,py,pz;
	intv vx,vy,vz;
	intv ox,oy,oz; // position offset for bullet spawn, center of rotation for cannon
	intv rox,roy,roz; // oxoyoz after rotation
	// FLAG have oxoyoz eventually be based on either global vars or guntype properties

	// orientation: x forward, y left, z up
	ox = 0;
	oy = -UPM*1/2;
	oz = -UPM*1/2;

	// Generate unit vector values for aim based on input.aim
	{//{{{

		intv tx, ty, tz;
		intu ct,st;
		intu cp,sp;
		intv stray;
		intv strayx, strayy, strayz;

		// At angle 0,0 camera z is facing down x axis
		tx = input->aimz;
		ty = -input->aimx;
		tz = input->aimy;

		NORMALIZE(tx,ty,tz) // Now t vars are <=1.0, can treat as intu

		// Apply inaccuracy
		// FLAG sloppy, do better if have time
		stray = ba_guntypes.accuracyconetangent[guntype];
		if(stray)
		{
			strayx=strayy=strayz=0;
			while(strayx+strayz+strayz==0) // to avoid 0 divide
			{
				strayx = UMV(2*irand()-UPM,stray);
				strayy = UMV(2*irand()-UPM,stray);
				strayz = UMV(2*irand()-UPM,stray);
			}
			NORMALIZE(strayx,strayy,strayz);
			strayx = UMU(strayx,stray);
			strayy = UMU(strayy,stray);
			strayz = UMU(strayz,stray);
			// now have a somewhat random sphere of radius stray to add to aim vector
			tx+=strayx;
			ty+=strayy;
			tz+=strayz;
			NORMALIZE(tx,ty,tz)
		}

		// Get cos and sin for theta and phi
		ANGTOCS(b->camtheta,ct,st)
		ANGTOCS(b->camphi,cp,sp)

		// Multiply aim vector by camera rotation matrix
		aimx = UMU(tx,UMU(ct,cp));
		aimx+= UMU(ty,-st);
		aimx+= UMU(tz,-UMU(ct,sp));
		aimy = UMU(tx,UMU(st,cp));
		aimy+= UMU(ty,ct);
		aimy+= UMU(tz,-UMU(st,sp));
		aimz = UMU(tx,sp);
		//  += 0*ty
		aimz+= UMU(tz,cp);

		// Rotate offset vector by cam rotation matrix
		rox = UMV(UMU(ct,cp),ox);
		rox+= UMV(-st,oy);
		rox+= UMV(-UMU(ct,sp),oz);

		roy = UMV(UMU(st,cp),ox);
		roy+= UMV(ct,oy);
		roy+= UMV(-UMU(st,sp),oz);

		roz = UMV(sp,ox);
		// += 0*oy
		roz+= UMV(cp,oz);


	}//}}}

	vx = UMV(aimx,evel)+b->coptervx;
	vy = UMV(aimy,evel)+b->coptervy;
	vz = UMV(aimz,evel)+b->coptervz;
	px = b->copterpx + rox;
	py = b->copterpy + roy;
	pz = b->copterpz + roz;

	battle_missleadd( &(b->md),
			px,py,pz,
			vx,vy,vz,
			missletype,
			ba_guntypes.misslelife[guntype]);
}
*/
//}}}

 // Gun Routines
 //{{{

  // GRAUTO
  //{{{
void battle_grauto(battledata* b, inputdatabattle* input, int gun)
{
	arsenal* a = b->gm->ars;
	int guntype = a->guntype[gun];

	a->gunphase[gun]-= BATTLEPERIOD;
	refire: // entrance point for it more than one shot should have fired during tick
	if(a->gunphase[gun]<=0)
	{
		if(input->triggerhold)
		{
			int* ammo = &(a->ammoboxammo[a->gunammobox[gun]]);
			if(*ammo>0)
			{
				int missletype = a->ammoboxmissletype[a->gunammobox[gun]];
				int time = BATTLEPERIOD+a->gunphase[gun];
				// time is how long ago the bullet should have fired, in seconds
				battle_shootmissle(b,input,guntype,missletype,time);

				// EFFECT gunshot based on missle type

				--(*ammo);
				debug_message("Ammo: %d\n",*ammo);//FLAGTE replace with HUD element
				a->gunphase[gun]+= ba_guntypes.reloadtime[guntype];
				goto refire;
			}
			else
			{
				// EFFECT dry gun click
				a->gunphase[gun] = 0;
			}
		}
		else
		{
			a->gunphase[gun] = 0;
		}
	}
}
  //}}}

  // GRCHARGE
  //{{{
void battle_grcharge(battledata* b, inputdatabattle* input, int gun)
{
	/*
	   gunphase acts as an indicator for how charged the cannon is.
	   0 = no charge
	   guns reloadtime = fully charged
	   In the future: have it so the gun will charge to a percentage of full
	   based on an input parameter, and have it so gun can't fire at less
	   than some minimum percentage (like 15%)
	   Also put in code for power consumption (so much to charge, a lesser
	   ammount to maintain a charge)
	   */
	arsenal* a = b->gm->ars;
	int guntype = a->guntype[gun];

	if(a->gunphase[gun]<=0)
	{// As of yet uncharged

		a->gunphase[gun] = 0;
		if(input->triggerpress)
		{
			int* ammo = &(a->ammoboxammo[a->gunammobox[gun]]);
			if(*ammo>0)
			{
				// Begin charging
				a->gunphase[gun] = BATTLEPERIOD;
				// EFFECT charge sound
			}
			else
			{
				// EFFECT some sort of sounds indicating no cells
			}
		}
	}
	else
	{ // Charged

		if(input->triggerhold)
		{ // Trigger held: continue charging

			intv rt = ba_guntypes.reloadtime[gun];

			a->gunphase[gun]+= BATTLEPERIOD;

			if(a->gunphase[gun]>rt)
			{
				a->gunphase[gun] = rt;
				// EFFECT fully charged sound
			}
			else
			{
				// EFFECT charge sound
			}
		}
		else
		{// Trigger released: fire cannon

			int* ammo = &(a->ammoboxammo[a->gunammobox[gun]]);
			if(*ammo>0) // redundant, but just in case
			{
				int missletype = a->ammoboxmissletype[a->gunammobox[gun]];
				// FLAG have special parameter based on phase/reloadtime
				battle_shootmissle(b,input,guntype,missletype,0);

				// EFFECT charge cannon fire

				--(*ammo);
				debug_message("Ammo : %d\n",*ammo);//FLAGTE replace with HUD element
				debug_message("Power: %f\n",((float)a->gunphase[gun])/ba_guntypes.reloadtime[guntype]);
				a->gunphase[gun] = 0;
			}
		}

	}

}
  //}}}

  // GRSEMI
  //{{{
void battle_grsemi(battledata* b, inputdatabattle* input, int gun)
{
	arsenal* a = b->gm->ars;
	int guntype = a->guntype[gun];

	a->gunphase[gun]-= BATTLEPERIOD;
	if(a->gunphase[gun]<=0)
	{
		if(input->triggerpress)
		{
			int* ammo = &(a->ammoboxammo[a->gunammobox[gun]]);
			if(*ammo>0)
			{
				int missletype = a->ammoboxmissletype[a->gunammobox[gun]];
				int time = BATTLEPERIOD+a->gunphase[gun];
				// time is how long ago the bullet should have fired, in seconds
				battle_shootmissle(b,input,guntype,missletype,time);

				// EFFECT gunshot based on missle type

				--(*ammo);
				debug_message("Ammo: %d\n",*ammo);//FLAGTE replace with HUD element
				a->gunphase[gun]+= ba_guntypes.reloadtime[guntype];
			}
			else
			{
				// EFFECT dry gun click
				a->gunphase[gun] = 0;
			}
		}
		else
		{
			a->gunphase[gun] = 0;
		}
	}
}
  //}}}

  // Init Gun Routine
  //{{{
void battle_initgr()
{
	ba_grfunctions[BA_GRAUTO] = battle_grauto;
	ba_grfunctions[BA_GRCHARGE] = battle_grcharge;
	ba_grfunctions[BA_GRSEMI] = battle_grsemi;
}
  //}}}

 //}}}

 // Init
//{{{
void battle_initWOWZERS() // FLAGTE change this stupid name once dynamic loading in place
{

	// Ammo classes
	//{{{
	{
		int ac;
		
		ac = BA_ACBULLET;
		ba_ammoclasses.radius[ac] = UPM/4;

		ac = BA_ACCHARGECELL;
		ba_ammoclasses.radius[ac] = UPM/2;

	}
	//}}}

	// Loads global data from external file (eventually, hardcoded atm) FLAG
	//  leave ammo classes as-is: ammo classes should be hard-coded, as they're tied
	//  pretty closely to hard-coded missle routines anyway.  If anything, make the values
	//  loaded from external file, but the number of classes and the philosophy behind
	//  the classes should be hard-coded.

	// Gun types
	//{{{
	{
		// FLAG load from external eventually
		int i = 0;
		int types = 3;
		ba_guntypes.routine = (int*)malloc(sizeof(int)*types);
		ba_guntypes.ammoclass = (int*)malloc(sizeof(int)*types);
		ba_guntypes.escapevelocity = (intv*)malloc(sizeof(intv)*types);
		ba_guntypes.accuracyconetangent = (intv*)malloc(sizeof(intv)*types);
		ba_guntypes.reloadtime = (intv*)malloc(sizeof(intv)*types);
		ba_guntypes.misslelife = (intv*)malloc(sizeof(intv)*types);

		// Vulcan
		ba_guntypes.routine[i] =             BA_GRAUTO;
		ba_guntypes.ammoclass[i] =           BA_ACBULLET;
		ba_guntypes.escapevelocity[i] =      UPM*40;
		ba_guntypes.accuracyconetangent[i] = UPM*1/40;
		ba_guntypes.reloadtime[i] =          UPM*1/200;
		ba_guntypes.misslelife[i] =          UPM*2;
		i++;

		// Charge Cannon
		//ba_guntypes.routine[i] =            BA_GRCHARGE;
		ba_guntypes.routine[i] =            BA_GRSEMI;
		ba_guntypes.ammoclass[i] =          BA_ACCHARGECELL;
		ba_guntypes.escapevelocity[i] =     UPM*30;
		ba_guntypes.accuracyconetangent[i] = 0;
		ba_guntypes.reloadtime[i] =         UPM*5/2;
		ba_guntypes.misslelife[i] =         UPM*10;
		i++;

		// Rifle
		ba_guntypes.routine[i] =            BA_GRSEMI;
		ba_guntypes.ammoclass[i] =          BA_ACBULLET;
		ba_guntypes.escapevelocity[i] =     UPM*50;
		ba_guntypes.accuracyconetangent[i] = UPM*1/256;
		ba_guntypes.reloadtime[i] =         UPM*3/8;
		ba_guntypes.misslelife[i] =         UPM*5;
		i++;


		ba_maxguntypes = types;
		if(types!=i)
		{
			// increment "types" if adding more!!
			debug_errormessage("GUN TYPES OFF, MALLOC ERROR, FIX!!!\n");
			exit(1);
		}

		// Check that guntype values don't exceed any maximums
		{
			int i;
			for(i=0;i<types;i++)
			{
				if(VDV(ba_guntypes.misslelife[i],ba_guntypes.reloadtime[i])>=MAXMISSLES*UPM)
				{
					debug_errormessage("Warning: Possible to exceed max missles with guntype %d.\n",i);
				}
				if(ba_guntypes.escapevelocity[i]/TPS+ba_ammoclasses.radius[ba_guntypes.ammoclass[i]] > MAXMISSLERADVELSUM)
				{
					debug_errormessage("Warning: Escape velocity + ammoclass radius exeed maximum sum allowed with guntype %d.\n",i);
				}
				if(VMV(ba_guntypes.escapevelocity[i],ba_guntypes.misslelife[i]) > MAXMISSLERANGE)
				{
					//debug_errormessage("Warning: Possible to exceed maximum missle range with guntype %d.\n",i);
				}

			}
		}

	}
	//}}}

	// Enemy types
	//{{{
	{
		// FLAG load from external eventually
		int i = 0;
		int types = 5;
		ba_enemytypes.vel = (intv*)malloc(sizeof(intv)*types);
		ba_enemytypes.absturnvel = (inta*)malloc(sizeof(inta)*types);
		ba_enemytypes.relturnvel = (inta*)malloc(sizeof(inta)*types);
		ba_enemytypes.maxhealth = (int*)malloc(sizeof(int)*types);
		ba_enemytypes.armor = (int*)malloc(sizeof(int)*types);
		ba_enemytypes.expfactor = (intu*)malloc(sizeof(intu)*types);
		ba_enemytypes.enemyroutine = (int*)malloc(sizeof(int)*types);
		ba_enemytypes.model = (int*)malloc(sizeof(int)*types);
		ba_enemytypes.ox = (intv*)malloc(sizeof(intv)*types);
		ba_enemytypes.oy = (intv*)malloc(sizeof(intv)*types);
		ba_enemytypes.oz = (intv*)malloc(sizeof(intv)*types);
		ba_enemytypes.rx = (intv*)malloc(sizeof(intv)*types);
		ba_enemytypes.ry = (intv*)malloc(sizeof(intv)*types);
		ba_enemytypes.rz = (intv*)malloc(sizeof(intv)*types);
		ba_enemytypes.bbr = (intv*)malloc(sizeof(intv)*types);

		//xx
		// Rifleman
		ba_enemytypes.vel[i] =          UPM*3/2;
		ba_enemytypes.absturnvel[i] =   PI2*1;
		ba_enemytypes.relturnvel[i] =   0;
		ba_enemytypes.maxhealth[i] =    1;
		ba_enemytypes.armor[i] =        0;
		ba_enemytypes.expfactor[i] =    UPM*1;
		ba_enemytypes.enemyroutine[i] = BA_ERTROOP;
		ba_enemytypes.model[i] =        RE_MRIFLEMAN;
		ba_enemytypes.ox[i] =           0;
		ba_enemytypes.oy[i] =           0;
		ba_enemytypes.oz[i] =           UPM*1;
		ba_enemytypes.rx[i] =           UPM*1/2;
		ba_enemytypes.ry[i] =           UPM*1/2;
		ba_enemytypes.rz[i] =           UPM*1;
		i++;

		// Tank
		ba_enemytypes.vel[i] =          UPM*7/8;
		ba_enemytypes.absturnvel[i] =   PI2*1/4;
		ba_enemytypes.relturnvel[i] =   0;
		ba_enemytypes.maxhealth[i] =    150;
		ba_enemytypes.armor[i] =        0;
		ba_enemytypes.expfactor[i] =    UPM*1;
		ba_enemytypes.enemyroutine[i] = BA_ERTANK;
		ba_enemytypes.model[i] =        RE_MTANK;
		ba_enemytypes.ox[i] =           0;
		ba_enemytypes.oy[i] =           0;
		ba_enemytypes.oz[i] =           UPM*5/4;
		ba_enemytypes.rx[i] =           UPM*5/2;
		ba_enemytypes.ry[i] =           UPM*3/2;
		ba_enemytypes.rz[i] =           UPM*5/4;
		i++;

		// Jeep
		ba_enemytypes.vel[i] =          UPM*2;
		ba_enemytypes.absturnvel[i] =   0;
		ba_enemytypes.relturnvel[i] =   PI2*1/4;
		ba_enemytypes.maxhealth[i] =    90;
		ba_enemytypes.armor[i] =        0;
		ba_enemytypes.expfactor[i] =    UPM*1;
		ba_enemytypes.enemyroutine[i] = BA_ERJEEP;
		ba_enemytypes.model[i] =        RE_MJEEP;
		ba_enemytypes.ox[i] =           -UPM*1;
		ba_enemytypes.oy[i] =           0;
		ba_enemytypes.oz[i] =           UPM*1;
		ba_enemytypes.rx[i] =           UPM*2;
		ba_enemytypes.ry[i] =           UPM*2;
		ba_enemytypes.rz[i] =           UPM*1;
		i++;

		// Missle
		ba_enemytypes.vel[i] =          UPM*5/2;
		ba_enemytypes.absturnvel[i] =   PI2*1/4;
		ba_enemytypes.relturnvel[i] =   0;
		ba_enemytypes.maxhealth[i] =    1;
		ba_enemytypes.armor[i] =        0;
		ba_enemytypes.expfactor[i] =    UPM*1;
		ba_enemytypes.enemyroutine[i] = BA_ERMISSLE;
		ba_enemytypes.model[i] =        RE_MAAMISSLE;
		ba_enemytypes.ox[i] =           UPM*1/2;
		ba_enemytypes.oy[i] =           0;
		ba_enemytypes.oz[i] =           0;
		ba_enemytypes.rx[i] =           UPM*1/4;
		ba_enemytypes.ry[i] =           UPM*3/4;
		ba_enemytypes.rz[i] =           UPM*1/4;
		i++;

		// Monolith
		ba_enemytypes.vel[i] =          0;
		ba_enemytypes.absturnvel[i] =   0;
		ba_enemytypes.relturnvel[i] =   0;
		ba_enemytypes.maxhealth[i] =    1;
		ba_enemytypes.armor[i] =        0;
		ba_enemytypes.expfactor[i] =    UPM*1;
		ba_enemytypes.enemyroutine[i] = BA_ERMISSLE;
		ba_enemytypes.model[i] =        RE_MMONOLITH;
		ba_enemytypes.ox[i] =           0;
		ba_enemytypes.oy[i] =           0;
		ba_enemytypes.oz[i] =           UPM*3;
		ba_enemytypes.rx[i] =           UPM*3;
		ba_enemytypes.ry[i] =           UPM*2;
		ba_enemytypes.rz[i] =           UPM*10;
		i++;

		ba_maxenemytypes = types;
		if(types!=i)
		{
			// increment "types" if adding more!!
			debug_errormessage("ENEMY TYPES OFF, MALLOC ERROR, FIX!!!\n");
			exit(1);
		}

		// Set bbr for each type
		{
			int i = 0;
			intv max;
			intv test;
			intv ox,oy,oz;
			for(i=0;i<types;i++)
			{
				max = 0;
				ox=ba_enemytypes.ox[i];
				oy=ba_enemytypes.oy[i];
				oz=ba_enemytypes.oz[i];

				if(ox>0) max=ba_enemytypes.rx[i]+ox;
				else max=ba_enemytypes.rx[i]-ox;
				if(oy>0) test=ba_enemytypes.ry[i]+oy;
				else test=ba_enemytypes.ry[i]-oy;
				if(test>max) max = test;
				if(oz>0) test=ba_enemytypes.rz[i]+oz;
				else test=ba_enemytypes.rz[i]-oz;
				if(test>max) max = test;

				ba_enemytypes.bbr[i] = max;

			}
		}

	}
	//}}}

	// Missle types
	//{{{
	{
		// FLAG load from external eventually
		int i = 0;
		int types = 3;
		ba_missletypes.gravmod = (intu*)malloc(sizeof(intu)*types);
		ba_missletypes.mass = (intv*)malloc(sizeof(intv)*types);
		ba_missletypes.ammoclass = (int*)malloc(sizeof(int)*types);
		ba_missletypes.routine = (int*)malloc(sizeof(int)*types);
		ba_missletypes.model = (int*)malloc(sizeof(int)*types);

		// Vulcan Bullet
		ba_missletypes.gravmod[i] =   UPM;
		ba_missletypes.mass[i] =      UPM*1/64;
		ba_missletypes.ammoclass[i] = BA_ACBULLET;
		ba_missletypes.routine[i] =   0;
		ba_missletypes.model[i] =     RE_MBULLET;
		i++;

		// Charge Cannon Cell
		ba_missletypes.gravmod[i] =   UPM;
		ba_missletypes.mass[i] =      UPM*1/32;
		ba_missletypes.ammoclass[i] = BA_ACCHARGECELL;
		ba_missletypes.routine[i] =   0;
		ba_missletypes.model[i] =     RE_MCHARGE;
		i++;

		// Antigrav Vulcan Bullet
		ba_missletypes.gravmod[i] =   -UPM;
		ba_missletypes.mass[i] =      UPM*1/64;
		ba_missletypes.ammoclass[i] = BA_ACBULLET;
		ba_missletypes.routine[i] =   0;
		ba_missletypes.model[i] =     RE_MBULLET;
		i++;

		ba_maxmissletypes = types;
		if(types!=i)
		{
			// increment "types" if adding more!!
			debug_errormessage("MISSLE TYPES OFF, MALLOC ERROR, FIX!!!\n");
			exit(1);
		}

	}
	//}}}

	battle_initgr();

}
//}}}

//}}}

