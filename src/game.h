

// Structs & Definitions
//{{{

 // Internal
//{{{
#ifdef GAME_C

  // "gamedata" : Data for an instance of game process
typedef struct
{
	int level;
	int money;
	int ammo;
	int state;
	int returnvalue;
} gamedata;

  // Gamestate enum
enum{
GAMESTATE_BATTLE,
GAMESTATE_SHOP,
GAMESTATE_RETURN,
GAMESTATE_FIN
};

#endif
//}}}

 // External
//{{{
#if defined(GAME_C) || defined(BATTLE_C) || defined(SHOP_C)

// "arsenal" : Contains data about guns and ammo presently in the copter
//{{{
typedef struct
{
	int guncount;		// Number of guns in arsenal
	int* guntype;		// Array, stores type of each gun
	int* gunammobox;	// Array, stores ammo box each gun is using
	intv* gunphase;		// Array, stores how much reload time remains for gun in seconds
	int ammoboxcount;	// Number of ammunition boxes in arsenal
	int* ammoboxmissletype;	// Array, stores the missletype for the ammo in each box
	int* ammoboxammo;	// Array, stores how much ammo is in each box
} arsenal;
//}}}

  // "gamemod" : Input for child processes. Allows modification of parts of gamedata
typedef struct
{
	int* money;
	int* ammo;
	int* returnvalue;
	int level;
	arsenal* ars;
} gamemod;

#endif
//}}}

//}}}

// Game Process
//{{{

#ifdef GAME_C
void* game_init(void*);
void game_loop(void*);
void game_pop(void*);
process gameprocess = {game_init,game_loop,game_pop};
#endif

#ifdef MAIN_C
extern process gameprocess;
#endif

//}}}

