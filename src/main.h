
// Structs & Definitions
//{{{
#if defined(MAIN_C) || defined(MAINMENU_C)

// "maindata" : Data for main process
typedef struct
{
	int state; // enum that dictates behavior of main loop
	void* interstatedata; // Stores data to be passed between states when state is changed
} maindata;

// Main State Handles
enum{
MASTATE_MENU,
MASTATE_NEWGAME,
MASTATE_TERMINATE
};

#endif
//}}}

// Main Process
//{{{
#ifdef MAIN_C
void* main_init(void*);
void main_loop(void*);
void main_pop(void* data){} // For sake of function pointer
process mainprocess = {main_init, main_loop, main_pop};
#endif
//}}}

