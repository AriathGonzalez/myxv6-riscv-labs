/*
Implement a time1 cmd that reports just elapsed time.
Output a usage msg and exit if the user does not enter cmd to be timed.
The following is needed:
1) Handle the cmd-line args input by the user.
2) Assemble args to pass by exec()
3) Use exec() in the child to exec prog passed as cmd to time1
4) Call wait() in the parent to wait until the child finishes.
5) Get the elapsed time by calling uptime() sys call in the parent before forking
   the child and after the wait returns
   
Note for each step:
1) Ensure that a cmd was inputted to be timed. If a cmd not inputted, exit with error code (1).
   An example of this usage is in user/mkdir.c, user/ls.c, user/kill.c, user/rm.c
2) Expects (cmd itself, array of strings that rep cmd's args)
   So, have to assemble argv in a way that 'time1' is not included as you want to exec matmul
3) For child, fork returns 0
4) To check for parent, remember that if fork is less than 0, error, if fork is 0, child.
   Example in user/init.c
5) Gonna use uptime() like hw1
*/

// Files needed
#include "kernel/types.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
	// Error: Did not enter program to be timed
	if (argc < 2) {
		fprintf(2, "Usage: time1 cmds...\n");
		exit(1);
	}
	
	// Get the current time before forking
	int startTime = uptime();
	int rc = fork();	// Fork child
	
	// Fork Failed
	if (rc < 0) {
		fprintf(2, "Fork failed, returning %d\n", rc);
		exit(1);
	}
	else if (rc == 0) {	// Child
		exec(argv[1], argv + 1);
		fprintf(2, "time1: exec %s failed\n", argv[1]);
		exit(1);
	}
	else {	// Parent
		/*
		Based on the expected output:
		Time: 28 ticks (matmul output)
		elapsed time: 28 ticks (time1 output)
		The child must run first (matmul)
		The parent runs second (time1)
		*/
		
		wait(0);	// Wait for child to terminate
		
		int endTime = uptime();
		int elapsedTime = endTime - startTime;
		fprintf(1, "elapsed time: %d ticks\n", elapsedTime);
	}
	
	exit(0);

}
