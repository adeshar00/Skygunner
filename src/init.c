
// Includes
//{{{

#include <unistd.h>

#define INIT_C
#include "intmath.h"
#include "battle.h"
#include "debug.h"

//}}}


// Functions
//{{{

void init_init()
{

	usleep(1000000); // simulate time spent loading stuff FLAG
	debug_init();
	intmath_init();
	battle_initWOWZERS(); // FLAG change this stupid fucking name once init renamed
}

//}}}
