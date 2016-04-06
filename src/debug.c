
// Includes
//{{{

#include <stdio.h>
#include <stdarg.h>

#define DEBUG_C
#include "debug.h"
#include "globals.h"

//}}}

int DEBUGON = 0;

// Functions
//{{{

 // Init
//{{{
#ifdef DEBUG
void debug_init()
{
	debug_message("Max Missle Range: %d meters\n", MAXMISSLERANGE/UPM);

	// Benchmarking init
	#if DEBUG_BENCHMARKING
	benchmarklist = 0;
	benchmarklisttp = &benchmarklist;
	#endif

}
#endif
//}}}

 // Benchmarking
//{{{
#if DEBUG_BENCHMARKING && defined(DEBUG)

  // Reset Node
//{{{
void debug_benchresetnode(benchnode* b)
{
	b->totalcpu = 0;
	b->totalwall = 0;
	b->samples = 0;
	b->highcpu = 0;
	b->highwall = 0;
	b->lowcpu = 3600;
	b->lowwall = 3600;
	return;
}
//}}}

  // Create
//{{{
void debug_benchcreate(char* name)
{

	// Check name length
	{
		char c;
		int i = 0;
		c = name[i];
		while(c!='\0')
		{
			c = name[++i];
		}
		if(i>=BENCHMARKNAMELENGTH)
		{
			debug_errormessage("Benchmark creation error, benchmark name \"%s\" is too long.\n");
			return;
		}
	}

	// Check to make sure name doesn't already exist
	benchnode* b = benchmarklist;
	while(b)
	{
		if(!strncmp(name,b->name,BENCHMARKNAMELENGTH))
		{
			debug_errormessage("Benchmark creation error, benchmark named %s already exists.\n");
			return;
		}
		b = b->next;
	}

	// Populate new node
	benchnode* new = (benchnode*)malloc(sizeof(benchnode));
	strncpy(new->name,name,BENCHMARKNAMELENGTH);
	new->name[BENCHMARKNAMELENGTH-1] = '\0';
	debug_benchresetnode(new);
	new->next = 0;

	// Add new node to list
	*benchmarklisttp = new;
	benchmarklisttp = &(new->next);

}
//}}}

  // Delete
//{{{
void debug_benchdelete(char* name)
{

	benchnode* b = benchmarklist;
	benchnode** prevp = &benchmarklist;
	while(b)
	{
		if(!strncmp(name,b->name,BENCHMARKNAMELENGTH))
		{
			*prevp = b->next;
			free(b);
			return;
		}
		prevp = &(b->next);
		b = b->next;
	}
	// FLAG error: benchmark deletion error no benchmark of that name
}
//}}}

  // Start
//{{{
void debug_benchstart(char* name)
{

	benchnode* b = benchmarklist;
	while(b)
	{
		if(!strncmp(name,b->name,BENCHMARKNAMELENGTH))
		{
			clock_gettime(CLOCK_MONOTONIC, &(b->startwall));
			clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(b->startcpu));
			return;
		}
		b = b->next;
	}
}
//}}}

  // Stop
//{{{
void debug_benchstop(char* name, char* comment, ...)
{

	struct timespec endcpu;
	struct timespec endwall;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &endcpu);
	clock_gettime(CLOCK_MONOTONIC, &endwall);

	benchnode* b = benchmarklist;
	while(b)
	{
		if(!strncmp(name,b->name,BENCHMARKNAMELENGTH))
		{
			double dtcpu;
			double dtwall;

			dtcpu = (endcpu.tv_sec-b->startcpu.tv_sec)*1000.0+\
			        (endcpu.tv_nsec-b->startcpu.tv_nsec)/1000000.0;
			dtwall = (endwall.tv_sec-b->startwall.tv_sec)*1000.0+\
			         (endwall.tv_nsec-b->startwall.tv_nsec)/1000000.0;

			b->totalcpu+= dtcpu;
			b->totalwall+= dtwall;
			++b->samples;

			if(dtcpu>b->highcpu) b->highcpu = dtcpu;
			if(dtcpu<b->lowcpu) b->lowcpu = dtcpu;
			if(dtwall>b->highwall) b->highwall = dtwall;
			if(dtwall<b->lowwall) b->lowwall = dtwall;

			// Store comment
			{
				va_list arg;
				va_start(arg, comment);
				vsnprintf(b->comment, BENCHMARKCOMMENTLENGTH, comment, arg);
				va_end(arg);
				b->comment[BENCHMARKCOMMENTLENGTH-1] = '\0';
			}


			return;
		}
		b = b->next;
	}
}
//}}}

  // Tally
//{{{
void debug_benchtally()
{
	static int ticks = 0;
	if(++ticks>=BENCHMARKTALLYTICKS)
	{
		ticks = 0;

		debug_message("+-------------+-----------------------+-----------------------+------- -- -\n");
		debug_message("| Benchmark   |       CPU Time        |      Real Time        |  \n");
		debug_message("|     (in ms) |   avg |  high |   low |   avg |  high |   low |   Comments\n");
		debug_message("+-------------+-------+-------+-------+-------+-------+-------+------- -- -\n");

		benchnode* b = benchmarklist;
		while(b)
		{
			int s = b->samples;
			debug_message("| %-12s|%6.2f |%6.2f |%6.2f |%6.2f |%6.2f |%6.2f | %s\n",
					b->name,
					b->totalcpu/s,
					b->highcpu,
					b->lowcpu,
					b->totalwall/s,
					b->highwall,
					b->lowwall,
					b->comment);
			debug_benchresetnode(b);
			b = b->next;
		}
		debug_message("+-------------+-------+-------+-------+-------+-------+-------+------- -- -\n\n");

	}
}
//}}}

#endif
//}}}

 // Debug Message
//{{{
#if DEBUG_MESSAGES && defined(DEBUG)

void debug_message(char s[], ...)
{
	if(DEBUGON)
	{
	va_list arg;

	va_start(arg,s);
#ifndef EMSCRIPTEN
	vfprintf(stderr,s,arg);
#else
	vfprintf(stdout,s,arg);
	// Leaving blank atm because prints lag like hell
#endif
	va_end(arg);

	fflush(stderr);
	}
}

#endif
//}}}

 // Debug Error Message
//{{{
void debug_errormessage(char s[], ...)
{
	va_list arg;

	va_start(arg,s);
	vfprintf(stderr,s,arg);
	va_end(arg);

	fflush(stderr);
}
//}}}

//}}}
