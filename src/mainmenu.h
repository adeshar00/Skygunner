

// Structs & Definitions
//{{{
#ifdef MAINMENU_C

typedef struct
{
	maindata* md; // Main menu process allowed to modify main process data directly
} mainmenudata;

typedef struct
{
	int command;	// enum corresponding to a command for the main thread
	void* pass;	// pointer to extra data to be passed alongside command
} mainmenuitd;

enum{ // Main Menu COMmand handlers
MMCOM_NULL,
MMCOM_NEWGAME,
MMCOM_TERMINATE
};

#endif
//}}}

// Mainmenu Process
//{{{

#ifdef MAINMENU_C
void* mainmenu_init(void*);
void mainmenu_loop(void*);
void mainmenu_pop(void*);
process mainmenuprocess = {mainmenu_init, mainmenu_loop, mainmenu_pop};
#endif

#ifdef MAIN_C
extern process mainmenuprocess;
#endif

//}}}

// Prototypes
//{{{

#ifdef UI_C
void mainmenu_newgame(int);
void mainmenu_terminate();
#endif

//}}}
