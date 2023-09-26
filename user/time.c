#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/pstat.h"	// For ru struct

// HW 2: Task 4
int main(int argc, char *argv[]) {
	struct rusage ru;
	
	if (argc < 2) {
		fprintf(2, "Usage: time1 cmds...\n");
		exit(1);
	}
	
	int startTime = uptime();
	int rc = fork();	
	
	if (rc < 0) {
		fprintf(2, "Fork failed, returning %d\n", rc);
		exit(1);
	}
	else if (rc == 0) {	
		exec(argv[1], argv + 1);
		fprintf(2, "time1: exec %s failed\n", argv[1]);
		exit(1);
	}
	else {	
		wait2(0, &ru);	// 0, not interested in status
				// &ru, get cpu time
				
		// Time reps all time intervals
		// CPU Time focuses on time CPU dedicates to executing a process
		// Spends time waiting for resources or other events that do not consume CPU time
		int endTime = uptime();
		int elapsedTime = endTime - startTime;
		fprintf(1, "elapsed time: %d ticks, cpu time: %d ticks, %d% CPU\n", elapsedTime, ru.cputime, (ru.cputime/elapsedTime) * 100);
	}
	
	exit(0);

}
