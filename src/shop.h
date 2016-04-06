

// Definitions
//{{{
#ifdef SHOP_C

enum{ // Command handles
SH_NULL,
SH_BUYAMMO,
SH_GETDATA,
SH_QUIT
};

#endif
//}}}

// Game Process
//{{{

#ifdef SHOP_C
void shop_start(gamemod*);
void* shop_init(void*);
void shop_loop(void*);
void shop_pop(void*);
process shopprocess = {shop_init,shop_loop,shop_pop};
#endif

#ifdef GAME_C
extern process shopprocess;
#endif

//}}}

// Prototypes
//{{{
#ifdef UI_C
int shop_buyammo();
void shop_leaveshop();
void shop_getdata();
#endif
//}}}

