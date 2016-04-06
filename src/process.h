

// Structs
//{{{

typedef struct process
{
	void* (*initfunction)(void*);
	void (*loopfunction)(void*);
	void (*popfunction)(void*);
} process;

#ifdef PROCESS_C
typedef struct processframe
{
	void* data;
	void (*loopfunction)(void*);
	void (*popfunction)(void*);
	struct processframe* parent;
} processframe;
#endif

// Used to typesafe stack data pointer without exposing processframe struct
typedef struct processstack
{
	void* top; // FLAG modify since this is unnecessary, just make "struct processstack" external
} processstack;

//}}}

// Variables
//{{{

#if defined(THREAD_C) || defined(MAIN_C) || defined(GAME_C) || defined(SHOP_C) || defined(BATTLE_C) || defined(MAINMENU_C)
extern processstack* pr_mainstack;
#endif

#if defined(THREAD_C) || defined(UI_C)
extern processstack* pr_uistack;
#endif

#ifdef PROCESS_C
processstack* pr_uistack;
processstack* pr_mainstack;
#endif

//}}}

// Prototypes
//{{{

void process_push(processstack*, process, void*);
void process_pop(processstack*);

#if defined(MAIN_C) || defined(UI_C)
processstack* process_createstack();
#endif

#ifdef THREAD_C
void process_toploop(processstack*);
void process_destroystack(processstack*);
#endif

//}}}









