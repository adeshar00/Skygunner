

// Structs
//{{{

 // "inputdatabattle" : Input data for the "battle" gamephase
#if defined(INPUT_C) || defined(BATTLE_C)
typedef struct
{
	// Movement Direction Inputs
	int forward;
	int back;
	int left;
	int right;
	int up;
	int down;
	// Trigger Inputs
	int triggerpress;
	int triggerhold;
	// Weapon Select Input
	int gunchange; // -1 if null, otherwise indicates which weapon to swap to
	int ammochange; // indicates how many ammobox slots to cycle through
	// Cannon Aiming Inputs
	intu aimx;
	intu aimy;
	intu aimz;
	intu aimlx;
	intu aimly;
	// View rotation Inputs
	intv rotatex;
	intv rotatey;
	int testpress;
	int testpress2;
} inputdatabattle;
#endif

//}}}

// Variables
//{{{

#ifdef INPUT_C
#define INPUT_KEYS 128
SDL_Event in_event;
int in_keyhold[INPUT_KEYS];	// Which keys are being held
int in_keypress[INPUT_KEYS];	// Which keys have been newly pressed since last tick
int in_leftmousehold;
int in_leftmousepress;
int in_rightmousehold;
int in_rightmousepress;
int in_cursorx;			// How many pixels between left side of screen and mouse
int in_cursory;			// How many pixels betweent top of screen and mouse
int in_deltamousewheel;		// How far mouse has been rotated in last tick
#endif
extern int in_keypress[]; // FLAG remove once menu code in place

//}}}

// Prototypes
//{{{

#ifdef UI_C
void input_init();
void input_menu();
void input_fetchmousecoords();
#endif

#ifdef BATTLE_C
inputdatabattle input_battle();
#endif

//}}}
