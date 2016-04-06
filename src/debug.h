
// Functionality Definitions:
//{{{
/* 
   These determine whether certain debug functionality is active or not.
   To activate or deactivate a certain set of functionality, set it's value to 1 or 0
*/


// If undefined, all debug functionality except error printing is disabled
//  Eventually have be passed as a compile parameter
// FLAGTE put below within a "indef" check, so all eval as zero if debug disabled?
#define DEBUG

// Determines if "debug_message()" calls do anything
#define DEBUG_MESSAGES 1

// Determines if benchmarking is on
#define DEBUG_BENCHMARKING 0

// Determines if hashed collision algorithm is checked against raw brute force method
#define DEBUG_RAWCOLLISIONTEST 1


//}}}

#ifndef DEBUG_C
extern int DEBUGON;
#endif

// Functionality
//{{{

 // Init
//{{{
#ifdef DEBUG
void debug_init();
#else
#define debut_init()
#endif
//}}}

 // Benchmarking
//{{{
#if DEBUG_BENCHMARKING && defined(DEBUG)

#include <stdlib.h>
#include <string.h>
#include <time.h>

// "Private"
#ifdef DEBUG_C

#define BENCHMARKNAMELENGTH 10
#define BENCHMARKCOMMENTLENGTH 30
#define BENCHMARKTALLYTICKS 50

// Stores info for individual benchmark tests
typedef struct benchnode
{
	char name[BENCHMARKNAMELENGTH];
	struct timespec startcpu;
	struct timespec startwall;
	double totalcpu;
	double totalwall;
	int samples;
	double highcpu;
	double lowcpu;
	double highwall;
	double lowwall;
	char comment[BENCHMARKCOMMENTLENGTH];
	struct benchnode* next;
} benchnode;

// Benchmark node list
benchnode* benchmarklist;	// Pointer to head of benchmark list
benchnode** benchmarklisttp;	// Pointer to tail pointer of list


// "Public"
#else

// Prototypes
void debug_benchcreate(char*);
void debug_benchdelete(char*);
void debug_benchstart(char*);
void debug_benchstop(char*,char*,...);
void debug_benchtally();

#endif

#else

#define debug_benchcreate(x);
#define debug_benchdelete(x);
#define debug_benchstart(x);
#define debug_benchstop(x,y,...);
#define debug_benchtally();

#endif
//}}}

 // Debug Message
//{{{
/*
   Calls to this function print messages to stderr only if DEBUG_MESSAGES is turned on.
   Otherwise the function call is just a null macro, adding no cost to overhead.
   */

#if DEBUG_MESSAGES && defined(DEBUG)
void debug_message(char[], ...);
#else
#define debug_message(x, ...)
#endif

//}}}

 // Error Message
//{{{

// Function for printing error messages, should always be visible
void debug_errormessage(char[], ...);

//}}}


//}}}
