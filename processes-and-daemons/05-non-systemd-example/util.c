#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

void die(int line_num, const char * format, ...)
{
    syslog(LOG_ERR, "Error at line number: %d", line_num);
    fprintf(stderr, "\nError at line number: %d\n", line_num);

    va_list vargs;
    va_list vargs2;

    va_start(vargs, format);
    va_copy(vargs2, vargs);

    vsyslog(LOG_ERR, format, vargs);
    vfprintf(stderr, format, vargs2);
    fprintf(stderr, "\n\n");

    va_end(vargs);
    va_end(vargs2);

    exit(EXIT_FAILURE);
}

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

