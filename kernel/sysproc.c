#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

// HW 4 Task 2, Modified so it does not allocate physical memory
// removes call to growproc()
uint64
sys_sbrk(void)
{
  int addr;
  int n;
  int newsz;
  struct proc *p;

  if(argint(0, &n) < 0)
    return -1;
    
  p = myproc();
  addr = myproc()->sz;
  /*
  if(growproc(n) < 0)
    return -1;
  */
  newsz = addr + n;
  if (newsz > TRAPFRAME)
  	return -1;
  p->sz = newsz;
  
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// return the number of active processes in the system
// fill in user-provided data structure with pid,state,sz,ppid,name
uint64
sys_getprocs(void)
{
  uint64 addr;  // user pointer to struct pstat

  if (argaddr(0, &addr) < 0)
    return -1;
  return(procinfo(addr));
}

// return number of bytes of free physical memory
uint64
sys_freepmem(void)
{
    // Multiply number of pages by 4096 to get the amount of free memory in bytes
    return(nfreepages() * PGSIZE);
}

/*
Initializes the unnamed semaphore at the address pointed to by sem. 
value - specifies initial value for the semaphore.
pshared - indicates whether this semaphore is to be shared btwn the threads
	of a proc, or btwn procs.
- has val o, then semaphore is shared btwn threads of a proc, and should be located
  at some addr that is visible to all threads.
- is nonzero, then semaphore is shared btwn procs, and should be located in a region
  of shared mem.
Initializing a semaphore that has already been initialized results in undefined behavior.

Returns 0 on success; on error -1
*/
uint64
sys_sem_init(void)
{
	uint64 semaddr;
	int pshared;
	int value;
	
	// Gets args
	if (argaddr(0, &semaddr) < 0 ||
	    argint(1, &pshared) < 0 ||
	    argint(2, &value) < 0)
	{
		return -1;
	}
	
	// If 'pshared' is 1, this means shared semaphores btwn procs
	if (pshared != 1)
		return -1
	
	// Get idx of unused location
	int semIdx = semalloc();
	if (semIdx < 0)
		return -1;
	
	// Initialize the count of the semaphore
	semtable.sem[semIdx].count = value;
	
	// Copying semaphore idx from kernel to user space.
	struct proc *p = myproc();
	if (copyout(p->pagetable, semaddr, (char*)&semIdx, sizeof(int)) < 0) {
		semdealloc(semIdx);
		return -1;
	}
	return 0;
}

/*
- Destroys the unnamed semaphore at the addr pointed to by sem.
- Only a semaphore that has been initialized by sem_init should be destroyed using sem_destroy
0 - success; -1 - error
*/
uint64
sys_sem_destroy(void)
{
	uint64 semaddr;
	
	if (argaddr(0, &semaddr) < 0)
		return -1;
	
	
	// Use copyin to get semaphore idx
	int semIdx;
	struct proc *p = myproc();
	if (copyin(p->pagetable, (char*)&semIdx, semaddr, sizeof(int)) < 0)
		return -1;
	
	// If semaphore is not valid return -1
	acquire(&semtable.sem[semIdx].lock);
	if (semtablemsem[semIdx].valid != 1) {
		release(&semtable.sem[semIdx].lock);
		return -1;
	}
	// Else deallocate semaphore
	semdealloc(semIdx);
	release(&semtable.sem[semIdx].lock);
	return 0;
}

/*
- Decrements (locks) the semaphore pointed to by sem. If the semaphore's val is greater
than zero, then the decrement proceeds, and the f()'s returns, immediately. If the 
semaphore currently has the val of 0, then the call blocks until either it becomes
possible to perform the decrement, or a signal handler interrupts the call.
0 - success; -1 - error
*/
uint64
sys_sem_wait(void)
{
	uint64 semaddr;
	
	if (argaddr(0, &semaddr) < 0)
		return -1;
	
	// Use copyin to get semaphore idx
	struct proc *p = myproc();
	int semIdx;
	if (copyin(p->pagetable, (char*)&semIdx, semaddr, sizeof(int)) < 0)
		return -1;
	
	acquire(&semtable.sem[semIdx].lock);
	if (semtable.sem[semIdx].valid != 1) {
		release(&semtable.sem[semIdx].lock);
		return -1;
	}
	
	while (semtable.sem[semIdx].count == 0)
		sleep((void*)&semtable.sem[semIdx], &semtable.sem[semIdx].lock);
	
	semtable.sem[semIdx].count -= 1;
	release(&semtable.sem[semIdx].lock);
	
	return 0;
}

/*
- Increments (unlocks) the semaphore pointed to by sem. If the semaphore's val
consequently becomes greater than 0, then another proc or thread blocked in a sem_wait()
call will be woken up and proceed to lock the semaphore.
0 - success; -1 - error
*/
uint64
sys_sem_post(void)
{
	uint64 semaddr;
	
	if (argaddr(0, &semaddr) < 0)
		return -1;
	
	// Use copyin to get semaphore idx
	struct prod *p = myproc();
	int semIdx;
	if (copyin(p->pagetable, (char*)&semIdx, semaddr, sizeof(int)) < 0)
		return -1;
	
	// If semaphore is not valid, return -1
	acquire(&semtable.sem[semIdx].lock);
	if (semtable.sem[semIdx].valid != 1){
		releasea(&semtable.sem[semIdx].lock);
		return -1;
	}
	
	semtable.sem[semIdx].count += 1;
	wakeup((void*)&semtable.sem[semIdx]);
	release(&semtable.sem[semIdx].lock);
	return 0;
	
	if (argptr(0, (void*)&sem, sizeof(sem)) < 0)
		return -1;
	
	if (sem_post(sem) < 0)
		return -1;
	
	return 0;
}
