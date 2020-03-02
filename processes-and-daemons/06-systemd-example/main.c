#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

static int running = 1;

void handle_signal(int signum) {
    syslog(LOG_NOTICE, "Signal %d received", signum);
    if (signum == SIGTERM) {
	syslog(LOG_NOTICE, "Exiting on SIGTERM");
    	running = 0;
    }
}

int main (int argc, char **argv)
{
        /* who am i */
        report_pgs("My");

	/* catch SIGTERM to exit cleanly */
	signal(SIGTERM, handle_signal);

        /* set the working directory to the root directory */
	/* so we don't unnecessarily hold inodes */
        if (chdir ("/") == -1) {
		die(__LINE__, "failed to change to root directory");
	}

	/* set user file creation mask to zero */
	umask(0);

        /* determine soft limit for open file descriptors */
        struct rlimit fd_limit;
        if (getrlimit(RLIMIT_NOFILE, &fd_limit) == -1) {
		die(__LINE__, "failed to get resource limit");
	}

        /* close all open files--overkill, but works */
        for (int i = 0; i < fd_limit.rlim_cur; i++)
                close (i);

        /* redirect fd's 0,1,2 to /dev/null */
        open ("/dev/null", O_RDWR);     /* stdin */
        dup (0);                        /* stdout */
        dup (0);                        /* stderror */

        /* do its daemon thing... */
        syslog(LOG_INFO, "Now running");

	int i = 0;	
	while (running == 1) {
		syslog(LOG_INFO, ((i++ % 2) == 0 ? "tick" : "tock"));
		sleep(2);
	}

        syslog(LOG_INFO, "Exiting");
        return EXIT_SUCCESS;
}

