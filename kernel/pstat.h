struct pstat {
  int pid;     // Process ID
  enum procstate state;  // Process state
  uint64 size;     // Size of process memory (bytes)
  int ppid;        // Parent process ID
  char name[16];   // Parent command name
  int priority;	// HW 3: Task 1
  uint64 cputime;
};	

// HW 2: Task 3
struct rusage {
	uint cputime;
};
