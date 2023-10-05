#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  struct pstat uproc[NPROC];
  int nprocs;
  int i;
  char *state;
  static char *states[] = {
    [SLEEPING]  "sleeping",
    [RUNNABLE]  "runnable",
    [RUNNING]   "running ",
    [ZOMBIE]    "zombie  "
  };

  nprocs = getprocs(uproc);
  if (nprocs < 0)
    exit(-1);

  // HW 3: Task 1, included priority and cputime
  printf("pid state    size   age  priority   cputime  ppid  name\n");
  for (i=0; i<nprocs; i++) {
    state = states[uproc[i].state];
    //pstate = uproc[i].state;
    //state = states[pstate];
    //if (pstate == RUNNABLE)
        //printf("%d   %s %l   %l         %d     %d      %d     %s\n", uproc[i].pid, state,
                   //uproc[i].size, uptime()-uproc[i].readytime, uproc[i].priority, uproc[i].cputime, uproc[i].ppid, uproc[i].name);
    //else
    printf("%d   %s %l             %d     %d       %d     %s\n", uproc[i].pid, state,
                   uproc[i].size, uproc[i].priority, uproc[i].cputime, uproc[i].ppid, uproc[i].name);
   }

  exit(0);
}

