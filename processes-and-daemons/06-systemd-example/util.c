#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

void report_pgs(char *name) {
	pid_t my_pid = getpid();
	pid_t my_ppid = getppid();
	pid_t my_pgid = getpgrp();
	pid_t my_psid = getsid(my_pid); // or use getsid(0) for current process

	syslog(LOG_INFO, "****************************************");
	syslog(LOG_INFO, "%s Process Information", name);
	syslog(LOG_INFO, "         Process ID: %05d", my_pid);
	syslog(LOG_INFO, "  Parent Process ID: %05d", my_ppid);
	syslog(LOG_INFO, "   Process Group ID: %05d", my_ppid);
	syslog(LOG_INFO, "         Session ID: %05d", my_psid);
}

