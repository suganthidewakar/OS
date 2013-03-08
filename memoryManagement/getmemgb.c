#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <stdio.h>
#include <proc.h>

WORD *getmemgb(unsigned nbytes){
	STATWORD ps;  
	struct mblock *alloc;
	disable(ps);
	alloc = getmem_new(nbytes+8);
	if(alloc == (WORD*) SYSERR){
		restore(ps);
		return( (WORD *)SYSERR);
	}
	struct pentry *pptr;
	pptr = &proctab[currpid];
	struct mblock *q;
	q = pptr->memAllocPtr;
	while(q->mnext != NULL)
	{
		q = q->mnext;
		if(q==NULL )
			break;
		if(q->mnext>=maxaddr)
			break;
	}
	q->mnext = (struct mblock*)alloc;
	q = q->mnext;
	q->mlen = nbytes+8;
	q->mnext = NULL;
	restore(ps);
	return alloc+1;
}