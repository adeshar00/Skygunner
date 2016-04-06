

// Definitions
//{{{
#if defined(BATTLE_C) || defined(INIT_C) || defined(GAME_C) || defined(SHOP_C)

// Ammo Classes
enum{
BA_ACBULLET,	// Just a chunk of metal
BA_ACCHARGECELL,// Cell that can be charged with energy by charge cannon
BA_AMMOCLASSCOUNT // Leave as last enum, serves to indicate # of ammo classes
};

// Gun Routines
enum{
BA_GRAUTO,	// Fires bullets automatically while trigger is held
BA_GRCHARGE,	// Holding trigger charges, releasing fires charged projectile
BA_GRSEMI,	// Fires one bullet per trigger pull, (maybe fires if held on reload)
BA_GUNROUTINECOUNT // Leave as last enum, serves to indicate # of gun routines
};

// Enemy Routines
enum{
BA_ERTROOP,	// Walk towards base, but shoot at copter if in range
BA_ERTANK,	// Rides towards base, attack base when in range
BA_ERJEEP,	// Rides towards copter, spawn missles when in range
BA_ERMISSLE,	// Fly towards copter, burst when in range
BA_ENEMYROUTINECOUNT // Leave as last enum, serves to indicate # of gun routines
};

#endif
//}}}

// Structs
//{{{

#ifdef BATTLE_C

// "missledata" : A buffer to store the data for up to MAXMISSLES missles.
//{{{
typedef struct
{
	int misslecount;	// number of missles currently in play
	intv px[MAXMISSLES];
	intv py[MAXMISSLES];	// xyz coordinates of missle position, in meters
	intv pz[MAXMISSLES];
	intv vx[MAXMISSLES];
	intv vy[MAXMISSLES];	// xyz components of missle velocity vector, meters per second
	intv vz[MAXMISSLES];
	intv lpx[MAXMISSLES];
	intv lpy[MAXMISSLES];	// xyz coords at last tick: used for render interpolation 
	intv lpz[MAXMISSLES];   //  and collision detection
	intv life[MAXMISSLES];	// how long before missle peters out, in seconds
	int type[MAXMISSLES];	// missletype of missle
	// void pointer, for special data related to type (i.e. target of homing missle)? FLAG
	// if adding or changing, make sure to modify missleadd and missledelete functions also!
} missledata;
//}}}

// "enemydata" : A buffer to store the data for up to MAXENEMIES enemies.
//{{{
typedef struct
{
	int enemycount;		// number of enemies currently in play
	intv px[MAXENEMIES];
	intv py[MAXENEMIES];	// xyz coordinates of enemy position, in meters
	intv pz[MAXENEMIES];
	intv lpx[MAXENEMIES];
	intv lpy[MAXENEMIES];	// xyz coords at last tick: used for render interpolation
	intv lpz[MAXENEMIES];	//  and collision detection
	inta theta[MAXENEMIES];
	inta phi[MAXENEMIES];
	intu ct[MAXENEMIES];
	intu st[MAXENEMIES];	// stored cos/sin values of theta/phi
	intu cp[MAXENEMIES];
	intu sp[MAXENEMIES];
	int health[MAXENEMIES];
	int type[MAXENEMIES];
} enemydata;
//}}}

// "battledata" : Stores data for an instance of the battle process
//{{{
typedef struct
{
	// Camera vars
	intv camx;
	intv camy;
	intv camz;
	inta camtheta;
	inta camphi;
	intv camlx;
	intv camly;
	intv camlz;
	inta camltheta;
	inta camlphi;
	// Copter vars
	intv copterpx;
	intv copterpy;
	intv copterpz;
	intv copterlpx;
	intv copterlpy;
	intv copterlpz;
	intv coptervx;
	intv coptervy;
	intv coptervz;
	int equippedgun;
	// Missle data
	missledata md;
	enemydata ed;
	// Effect list
	neweffectlistnode* neweffectlisthead;
	neweffectlistnode** neweffectlisttailpointer;
	// Pointers to part of Game data
	gamemod* gm;
	// Tick count (since battle start)
	int tickcount;
} battledata;
//}}}

// "enemytypedata" : Struct of arrays of all enemytype parameters
//{{{
typedef struct
{
	intv* vel;		// Velocity
	inta* absturnvel;	// Turn velocity, absolute (like a tank)
	inta* relturnvel;	// Turn velocity, factored by speed (like a car)
	int* maxhealth;		// Max health attribute, see damage doco for more info
	int* armor;		// Armor attribute, see damage doco for more info
	intu* expfactor;	// See damage documentation for more info
	int* enemyroutine;	// Which script is used for enemy behavior
	int* model;		// Which model to render
	intv* ox;
	intv* oy;		// Offset for center of collision ellipsoid
	intv* oz;
	intv* rx;
	intv* ry;		// Dimensions of collision ellipsoid
	intv* rz;
	intv* bbr;		// Bounding box radius, used in collision detection
	// After adding new data, add malloc and valuess to battle init
} enemytypedata;
//}}}

#endif

#if defined(BATTLE_C) || defined(GAME_C)

// "missletypedata" : Struct of arrays of all missletype parameters
//{{{
typedef struct
{
	intu* gravmod;
	intv* mass;
	int* ammoclass;
	int* routine;
	int* model;
	// if adding or changing, make sure to modify battle_init also!
} missletypedata;
//}}}

// "guntypedata" : Struct of arrays of all guntype parameters
//{{{
typedef struct
{
	int* routine;			// which script to use when generating missles
	int* ammoclass;			// which class of ammo gun takes
	intv* escapevelocity;		// velocity missle fires off at
	intv* accuracyconetangent;	// tan of angle between center of cone and side
	intv* reloadtime;		// how much time in seconds between missle shots
	intv* misslelife;		// how long, in seconds, before missle dies
	// if adding or changing, make sure to modify battle_init also!
} guntypedata;
//}}}

// "ammoclassdata" : Struct of arrays of all ammoclass parameters
//{{{
typedef struct
{
	intv radius[BA_AMMOCLASSCOUNT];
} ammoclassdata;
//}}}

#endif

#ifdef INIT_C
typedef struct missletypedata missletypedata;
typedef struct guntypedata guntypedata;
#endif

//}}}

// Variables
//{{{

 // "Public" Global Vars
#ifdef BATTLE_C
#define VISIBLE
#else
#define VISIBLE extern
#endif

#if defined(BATTLE_C) || defined(INIT_C)
//VISIBLE int example;
#endif

#undef VISIBLE


 // "Private" Global Vars
#ifdef BATTLE_C
void (*ba_grfunctions[BA_GUNROUTINECOUNT])(battledata*,inputdatabattle*,int);
enemytypedata ba_enemytypes;
int ba_maxenemytypes;
missletypedata ba_missletypes;
int ba_maxmissletypes;
guntypedata ba_guntypes;
int ba_maxguntypes;
ammoclassdata ba_ammoclasses;
#endif

//}}}

// Prototypes
//{{{
void battle_initWOWZERS();
//}}}

// Battle Process
//{{{

#ifdef BATTLE_C
void* battle_init(void*);
void battle_loop(void*);
void battle_pop(void*);
process battleprocess = {battle_init,battle_loop,battle_pop};
#endif

#ifdef GAME_C
extern process battleprocess;
#endif

//}}}

