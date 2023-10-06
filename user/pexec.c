#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/pstat.h"	

// HW 3: Task 3
int main(int argc, char *argv[]) {
	if (argc < 3) {
		fprintf(2, "Usage: pexec cmds...\n");
		exit(1);
	}
	
	int priority = atoi(argv[1]);
	int rc = fork();	
	
	if (rc < 0) {
		fprintf(2, "Fork failed, returning %d\n", rc);
		exit(1);
	}
	else if (rc == 0) {	
		setpriority(priority);	// Set priority of child
		exec(argv[2], argv + 2);
		
		fprintf(2, "pexec: exec %s failed\n", argv[2]);
		exit(1);
	}
	else {	
		setpriority(priority);	// Set priority of parent
		wait(0);
	}
	
	exit(0);

}
