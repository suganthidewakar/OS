#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <stdio.h>
#include <proc.h>

SYSCALL freememgb(struct mblock *block, unsigned size)
{
	STATWORD ps;  
	int i,j;
	struct pentry *pptr;
	pptr = &proctab[currpid];
	struct mblock *q,*p,*temp;
	p = pptr->memAllocPtr;
	q = p->mnext;
	int found = 0;
	if (size==0 || (unsigned)block>(unsigned)maxaddr
	    || ((unsigned)block)<((unsigned) &end))
		return(SYSERR);
	disable(ps);
	while(q != NULL){
		if(q == (struct mblock*)block-1){
			found = 1;
			if(size != q->mlen-8){
				restore(ps);
				return(SYSERR);
			}
			temp = q->mnext;
			break;
		}
		p = q;
		q = q->mnext;
	}
	if(found == 0){
		restore(ps);
		return(SYSERR);
	}
	unsigned int ret = freemem_new(q,q->mlen);
	p->mnext = temp;
	restore(ps);
	return(ret);
}