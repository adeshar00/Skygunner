
// Includes
//{{{

#include <stdlib.h>

#define PROCESS_C
#include "process.h"

//}}}


// Functions for process stack
//{{{

/*
   Yeah I know, the use of the "processstack" struct is pretty goddamn ugly, but only
    a few lines of code have to deal with the guts of it and they're all within this
    file, so externally it's nice and typesafe. Don't have to expose the processframe
    struct to external files, and the functions don't have a second void* argument
   */

void process_push(processstack* ps, process p, void* input)
{
	processframe* pf = (processframe*)malloc(sizeof(processframe));

	pf->data = p.initfunction(input);
	pf->loopfunction = p.loopfunction;
	pf->popfunction = p.popfunction;
	pf->parent = (processframe*)(ps->top);

	ps->top = (void*)pf;

}

void process_pop(processstack* ps)
{
	processframe* pf = (processframe*)(ps->top);
	pf->popfunction(pf->data);
	ps->top = (void*)(pf->parent);
	free(pf);
}

processstack* process_createstack()
{

	processstack* ps = (processstack*)malloc(sizeof(processstack));
	ps->top = 0;

	return ps;
}

void process_destroystack(processstack* ps)
{
	while(ps->top)
	{
		processframe* pf = (processframe*)(ps->top);
		pf->popfunction(pf->data);
		ps->top = (void*)(pf->parent);
		free(pf);
	}
	free(ps);
}

void process_toploop(processstack* ps)
{
	processframe* top = (processframe*)(ps->top);
	top->loopfunction(top->data);
}

//}}}

