#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  /*
  * Remember, Page Faults:
  If present bit is 1, pg is present in phsical mem
  If present bit is 0, pg is not in mem but on disk => page fault (page miss)
  
  // HW 4 Task 3
  // SCAUSE register
  // Exception Code 13: Load page fault
  // Exception Code 15: Store/AMO page fault
  */
  if (r_scause() == 13 || r_scause() == 15) {
  	uint64 pgaddr;
  	char *mem;
  	// HW 5 Task 1b -----------------
  	int i;
  	int validaddr;
  	struct mmr *pmmr;
  	
  	// stval register holds the faulting address when a trap or exception occurs
  	// Get faulting address and round down to page boundary
  	pgaddr = PGROUNDDOWN(r_stval());	
  	
  	// Check if faulting address is beyond proc's valid virtual addr space
  	if (pgaddr >= p->sz) {
  		// HW 5 Task 1b ------------------------
  		pmmr = p->mmr;
  		validaddr = 0;
  		// Loop through all mapped memory regions in proc
  		for (i = 0; i < MAX_MMR; i++) {
  			// Checks if faulting address falls w/in a memory mapped region
  			if (pmmr[i].valid && pgaddr >= pmmr[i].addr && 
  				pgaddr < pmmr[i].addr + pmmr[i].length) {
  				// pgaddr falls in valid mapped region, now check permission
  				if (((r_scause() == 13) && !(pmmr[i].prot & PTE_R)) ||
  				   ((r_scause() == 15) && !(pmmr[i].prot & PTE_W))) {
  				   	printf("usertrap(): permission: %p\n", pgaddr);
  				   	p->killed = 1;
  				   	exit(-1);
  				}
  				validaddr = 1;
  				break;
  			}
  			if (!validaddr) {
  				printf("usertrap(): invalid virtual address: %p\n", pgaddr);
  				p->killed = 1;
  				exit(-1);
  			}
  		}
  		
  	}
  	// HW 5 Task 1b -----------------------
  	
  	mem = kalloc();	// allocation physical memory frame
  	if (mem == 0) {
  		printf("kalloc() failed\n");
  		p->killed = 1;
  		exit(-1);
  	}
  	/*
  	void *memset(void *str, int c, size_t n)
  	Copies the character c to the first n characters of the string pointed to
  	by the argument str.
  	Ex: str = 'This is string.h library function'
  	memset(str, '$', 7);
  	Output: $$$$$$$ string.h library function
  	
  	In this case, the initializes a new allocated physical memory frame 'mem' 
  	to all zeroes. Used to set all 4096 bytes in the memory regioni to the value 0.
  	
  	In other words, it's used to clear the block of physical memory that is associated 
  	w/ the faulting virtual page.
  	
  	*/
  	memset(mem, 0, PGSIZE);
  	
  	/*
	int
	mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
	// Creates translations from 'va' (virtual addr) to pa (physical addr) in 
	// existing page table 'pagetable'
	// Returns 0 if successful, -1 if not
	
	// Get starting addr 
	// Get ending addr (which is starting addr if size = 1)
	// For each page...
	// Get i1 row addr (using walkpgdir)
	// Make sure i1 row not used already
	// Write pa in i1 and mark as valid, w/ required permissions
	
  	Map Virtual Page to Physical Memory
  	Insert mapping into proc's pagetable
  	*/
  	if (mappages(p->pagetable, pgaddr, PGSIZE, (uint64)mem,
  		PTE_R|PTE_W|PTE_X|PTE_U) != 0) {
  		printf("mappages() failed\n");
  		kfree(mem);
  		p->killed = 1;
  		exit(-1);	
  		
  	}
  	
  	// HW 5 Extra Credit
  	struct mmr_list *mmr_list;
  	struct mmr_node *mmr_node;
  	
  	pmmr = p->mmr;
  	
  	for (int i = 0; i < MAX_MMR; i++) {
  		if (pmmr[i].valid && pgaddr >= pmmr[i].addr && 
  				pgaddr < pmmr[i].addr + pmmr[i].length) {
  			mmr_list = get_mmr_list(pmmr->mmr_family.listid);
  			acquire(&mmr_list->lock);
  			mmr_node = pmmr[i].mmr_family.next;
		  	while (mmr_node != &pmmr->mmr_family) {
		  		if (mmr_node->proc != p) {
			  		if (mappages(mmr_node->proc->pagetable, pgaddr, PGSIZE, (uint64)mem,
				  		PTE_R|PTE_W|PTE_X|PTE_U) != 0) {
				  		printf("mappages() failed\n");
				  		kfree(mem);
				  		p->killed = 1;
				  		exit(-1);	
			  		
			  		}
			  	}
		  		mmr_node = mmr_node->next;

		  	}
		        release(&mmr_list->lock);
		}
	}
  
  }
  else if(r_scause() == 8){
    // system call

    if(p->killed)
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sstatus &c registers,
    // so don't enable until done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // ok
  } else {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }

  if(p->killed)
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to trampoline.S
  w_stvec(TRAMPOLINE + (uservec - trampoline));

  // set up trapframe values that uservec will need when
  // the process next re-enters the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if(cpuid() == 0){
      clockintr();
    }
    
    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}

