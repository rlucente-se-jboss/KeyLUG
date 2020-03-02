#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

void report_pgs(char *name) {
	pid_t my_pid = getpid();
	pid_t my_ppid = getppid();
	pid_t my_pgid = getpgrp();
	pid_t my_psid = getsid(my_pid); // or use getsid(0) for current process

	printf("****************************************\n");
	printf("%s Process Information\n", name);
	printf("         Process ID: %05d\n", my_pid);
	printf("  Parent Process ID: %05d\n", my_ppid);
	printf("   Process Group ID: %05d\n", my_ppid);
	printf("         Session ID: %05d\n", my_psid);
}

