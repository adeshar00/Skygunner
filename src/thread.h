
// Internal Data
//{{{

#ifdef THREAD_C
#define THREADSEEPHASE_H
#endif

//}}}

// External Data
//{{{

#ifndef THREAD_C
extern void* th_itd;
void thread_lockitdmutex();
void thread_unlockitdmutex();
#endif

#ifdef UI_C
#define THREADSEEPHASE_H
#define THREADBATTLEUI_H
int thread_checkforgamephasechange();
int thread_getgamephase();
void thread_terminateui();
void thread_waitforuiticker();
int thread_gettime();
void thread_uiloop();
#endif

#ifdef MAIN_C
#define THREADALTERPHASE_H
void thread_init();
void thread_startui();
void thread_startmainloop();
void thread_terminatemain();
#endif

#ifdef MAINMENU_C
#define THREADALTERPHASE_H
#define THREADMENU_H
#endif

#ifdef GAME_C
#define THREADALTERPHASE_H
#endif

#ifdef BATTLE_C
#define THREADALTERPHASE_H
#define THREADBATTLEUI_H
void thread_restartbattleticker();
void thread_waitforbattleticker();
int thread_gettime();
int thread_getbattleperiod();
#endif

#ifdef SHOP_C
#define THREADALTERPHASE_H
#define THREADMENU_H
#endif

#ifdef INPUT_C
void thread_lockinputmutex();
void thread_unlockinputmutex();
#endif

//}}}


// ITD (InterThread Data) structs
//{{{

#if defined(BATTLE_C) || defined(UI_C) || defined(RENDER_C)
typedef struct neweffectlistnode
{
	int type;
	intv px;
	intv py;
	intv pz;
	struct neweffectlistnode* next;
} neweffectlistnode;
typedef struct
{
	int starttime;
	int maintickcount;
	float camx;
	float camy;
	float camz;
	float camlx;
	float camly;
	float camlz;
	int camtheta;
	int camphi;
	int camltheta;
	int camlphi;
	int renderobjectcount;
	int romodelnum[MAXRENDEROBJECTS]; // 'ro' prefix for "render object"
	int ropx[MAXRENDEROBJECTS];
	int ropy[MAXRENDEROBJECTS];
	int ropz[MAXRENDEROBJECTS];
	int rotheta[MAXRENDEROBJECTS];
	int rophi[MAXRENDEROBJECTS];
	intv hudevel;
	neweffectlistnode* neweffectlisthead;
	neweffectlistnode** neweffectlisttailpointer;
} battlerenderbuffer;
typedef struct
{
	int freshflag;
	int renderbufferid;
	battlerenderbuffer brb[2];
} battleitd;
#endif

#if defined(SHOP_C) || defined(UI_C)
typedef struct
{
	// Menu interaction data
	int command;
	void* pass;
	// Display data
	int ammo;
	int money;
} shopitd;
#endif

//}}}


// Shared Functionality
//{{{

#ifdef THREADALTERPHASE_H
#define THREADSEEPHASE_H
void thread_changegamephase(int);
#endif

#ifdef THREADSEEPHASE_H
enum{
GAMEPHASE_INTRO,
GAMEPHASE_MAINMENU,
GAMEPHASE_LOADING,
GAMEPHASE_BATTLE,
GAMEPHASE_SHOP,
GAMEPHASE_GAMEOVER,
GAMEPHASE_TERMINATE // This should be last in list- used to determine number of phases
};
int thread_getgamephase();
#endif

#ifdef THREADMENU_H
void thread_menuwaitforui();
void thread_menureleaseui();
void thread_menuprocess();
#endif

#ifdef THREADBATTLEUI_H
#define THREADBATTLEUI_H
void thread_lockbvd();
void thread_unlockbvd();
#endif

//}}}
