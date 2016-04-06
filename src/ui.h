
// Structs
//{{{

#if defined(UI_C) || defined(RENDER_C)

// Effect Data
typedef struct
{
	// modifying effect pipeline is currently a pain in the ass, have to update:
	//  this struct
	//  battle_effectadd in battle.c (and all calls to it if modifying arguments)
	//  the neweffectlistnode struct in thread.h
	//  ui_battleTYPEeffectadd in ui.c
	//  the effect update code in ui.c, if necessary
	//  effect render code
	//  ... and possibly something I'm forgetting :X
	int chunktype[MAXCHUNKEFFECTS];
	int chunklife[MAXCHUNKEFFECTS];
	intv chunkpx[MAXCHUNKEFFECTS];
	intv chunkpy[MAXCHUNKEFFECTS];
	intv chunkpz[MAXCHUNKEFFECTS];
	int chunkcount;
	int gasattribute[MAXGASEFFECTS];
	int gascount;
	// FLAG insert data for sound when necessary: above thingies are visual
} effectdata;

#endif

//}}}

// Variables & Constants
//{{{
#ifdef UI_C

#define VOLMIN 0  // FLAG delete? or play with making options menu first...
#define VOLMAX 100
int ui_volume;

// maybe give these all the same prefix?
int ui_clipdistancenear=1; // in meters
int ui_clipdistancefar=1000;

// "Null" functions, since function pointers can't be 0
void ui_null(void* data){}
void* ui_nullpush(void* input){return 0;}

float TESTcx=0;
float TESTcy=0;
float TESTcz=10;
float TESTtheta=0;
float TESTphi=0;

#endif

#if defined(RENDER_C) || defined(INPUT_C)
extern int ui_clipdistancenear;
extern int ui_clipdistancefar;
#endif

//}}}

// Prototypes
//{{{

#ifdef THREAD_C
void ui();
#endif

//}}}

