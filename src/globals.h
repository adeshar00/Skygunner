//FLAG replace UPM with UPW for "units per whole number".  would put in intmath but gets replaced
// Includes
//{{{
#include "intmath.h"
//}}}

// Definitions
//{{{


// Buffer sizes
#define MAXMISSLES 512
#define MAXENEMIES 512
#define MAXRENDEROBJECTS (MAXMISSLES+MAXENEMIES)
#define MAXGASEFFECTS 1024
#define MAXCHUNKEFFECTS 1024

// How many milliseconds between UI ticks FLAG put in seconds, convert to ms on the fly?
#define UIPERIOD 10

// Max lag cusion for UI ticker, see tick.c for more info
#define UIBUFFER 5

// Ticks Per Second
//  Note: UPM defined in intmath.h
//  16 ticks per second FLAG SET BACK TO 4
#define TPS (1<<6)

// Seconds between ticks for battle loop
#define BATTLEPERIOD (UPM/TPS)

// How much of a cushion battle ticker has 
#define BATTLEBUFFER (BATTLEPERIOD/3)

#define GRAVITY (-10*UPM)

// Modifier of camera rotation speed
#define ROTATEMOD (2*UPM/5)

// Maximum radii and velocities, for collision algorithm
// {{{
//
//  Note: limits only apply to x/y dimensions, z values that exceed these maximums
//   are kosher if there's no chance they'll dip into x or y
//   E.X. Tall enemy buildings, or missles that've been pulled past maxvel by gravity, are ok
//  These values should be high enough that they're never hit, but not so high as to make
//   the collision cell width enormous

// Collision cell shift
// How many bits to shift a position for determining collision grid hash index.
// Numeric value added to fractional bits should be log_2(whatever the desired collision
//  cell width is in meters)
// See collision cell width check below to see how large it needs to be
#define COLLISIONCELLWIDTHSHIFT (_FB+4)

// Width of a collision grid cell, in standard units (meters*UPM)
//  Value should be dependent on the above shift
#define COLLISIONCELLWIDTH (1<<COLLISIONCELLWIDTHSHIFT)

// Collision grid width shift
//  log_2(collision grid widith in cells)   (see below)
#define COLLISIONGRIDWIDTHSHIFT (5)

// How wide collision grid is, in cells
#define COLLISIONGRIDWIDTHINCELLS (1<<COLLISIONGRIDWIDTHSHIFT)

// How wide collision grid is, in standard units (meters*UPM)
#define COLLISIONGRIDWIDTH (COLLISIONGRIDWIDTHINCELLS*COLLISIONCELLWIDTH)

// Maximum value that a missles radius+velocity/tps can be
#define MAXMISSLERADVELSUM (5*UPM)

// Maximum copter velocity
#define MAXCOPTERVELOCITY (20*UPM)

// Maximum radius of the area that a missle can cover in one tick
#define COLLISIONMISSLERADIUS (MAXMISSLERADVELSUM+(MAXCOPTERVELOCITY/TPS))

// Maximum distance a missle can be from the copter
#define MAXMISSLERANGE (COLLISIONGRIDWIDTH/2)

// Check that max missle range won't overflow (otherwise maxmisslerange check won't work)
#if ( COLLISIONCELLWIDTHSHIFT+COLLISIONGRIDWIDTHSHIFT-_FB > 16 )
#error "Maxmisslerange^2 overflow: either adjust collision grid size, or change the max missle range test in the collision detection code."
#endif

// }}}


//}}}

// Global Variables
//{{{
#ifdef GLOBALS_C
#define VISIBLE
#else
#define VISIBLE extern
#endif

VISIBLE int g_windowwidth;
VISIBLE int g_windowheight;
VISIBLE int g_windowsquare;

// Tangent of angle between middle of screen and edge of window square
VISIBLE intv g_viewangletangent;


#undef VISIBLE
//}}}

// Prototypes
//{{{
#ifdef MAIN_C
void globals_init();
#endif
//}}}

