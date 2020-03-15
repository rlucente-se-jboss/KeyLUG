#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

static void log_message(int priority, FILE *stream, char *format, va_list vargs)
{
    va_list vargs2;
    va_copy(vargs2, vargs);

    vsyslog(priority, format, vargs);
    vfprintf(stream, format, vargs2);
    fprintf(stream, "\n");

    va_end(vargs2);
}

void die(int line_num, char *format, ...) {
    syslog(LOG_ERR, "Error at line number: %d", line_num);
    fprintf(stderr, "\nError at line number: %d\n", line_num);

    va_list vargs;
    va_start(vargs, format);
    log_message(LOG_ERR, stderr, format, vargs);
    va_end(vargs);

    fprintf(stderr, "\n");

    exit(EXIT_FAILURE);
}

void log_info(char *format, ...) {
    va_list vargs;
    va_start(vargs, format);
    log_message(LOG_INFO, stdout, format, vargs);
    va_end(vargs);
}

void report_pgs(char *name) {
	pid_t my_pid = getpid();
	pid_t my_ppid = getppid();
	pid_t my_pgid = getpgrp();
	pid_t my_psid = getsid(my_pid); // or use getsid(0) for current process

	log_info("****************************************");
	log_info("%s Process Information", name);
	log_info("         Process ID: %05d", my_pid);
	log_info("  Parent Process ID: %05d", my_ppid);
	log_info("   Process Group ID: %05d", my_ppid);
	log_info("         Session ID: %05d", my_psid);
}

