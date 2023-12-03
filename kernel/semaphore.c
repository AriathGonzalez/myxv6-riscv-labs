#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "spinlock.h"

struct semtab semtable;

void seminit(void)
{
	initlock(&semtable.lock, "semtable");
	for (int i = 0; i < NSEM; i++)
		initlock(&semtable.sem[i].lock, "sem");
};

// Return the idx of an unused location in the semaphore table
// or returns -1 if there is no empty location.
int semmalloc(void)
{
	acquire(&semtable.lock);
	for (int i = 0; i < NSEM; i++) {
		if (!semtable.sem[i].valid) {
			semtable.sem[i].valid = 1;
			release(&semtable.lock);
			return i;
		}
	}
	release(&semtable.lock);
	return -1;
}

// Takes a semaphore idx as an arg and invalidates that entry in the 
// semaphore table. [Concurrency control required]
void semdealloc(int idx)
{
	acquire(&semtable.sem[idx].lock);
	if (idx >= 0 && idx < NSEM)
		semtable.sem[idx].valid = 0;
	release(&semtable.sem[i].lock);
}
