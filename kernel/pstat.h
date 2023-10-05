struct pstat {
  int pid;     // Process ID
  enum procstate state;  // Process state
  uint64 size;     // Size of process memory (bytes)
  int ppid;        // Parent process ID
  char name[16];   // Parent command name
  uint priority;	// HW 3: Task 1
  uint64 cputime;	// HW 2
  uint64 readytime; 	// HW 3: Task 2, time (in ticks from sys boot) at which process became RUNNABLE
};	

// HW 2: Task 3
struct rusage {
	uint cputime;
};
