#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/resource.h>
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

        /* create new process */
        pid_t pid = fork ();
        if (pid == -1) {
		die(__LINE__, "failed to fork");
	} else if (pid > 0) {
		report_pgs("Parent");

                /* make the child an orphan */
		syslog(LOG_INFO, "Parent exiting ...");
                exit(EXIT_SUCCESS);
	}

	/* Child process gets here */
	report_pgs("Initial Child");

        /* create new session and process group */
        if (setsid() == -1) {
		die(__LINE__, "failed to create new session");
	}

        /* wait a second for reaping by process 1 */
	sleep(1);
	report_pgs("Reaped Child");

	/* catch SIGHUP on second fork so grandchild doesn't exit */
	/* catch SIGTERM to exit cleanly */
	signal(SIGHUP, handle_signal);
	signal(SIGTERM, handle_signal);

	/* fork again to prevent a controlling terminal from being opened */
	pid = fork();
	if (pid == -1) {
		die(__LINE__, "failed to fork grandchild");
	} else if (pid > 0) {
                /* make the grandchild an orphan */
		syslog(LOG_INFO, "Reaped child exiting ...");
                exit(EXIT_SUCCESS);
	}

	report_pgs("Initial Grandchild");

        /* wait a second for reaping by process 1 */
	sleep(1);
	report_pgs("Reaped Grandchild");

	/* wait a few seconds to see if SIGHUP is caught */
        sleep(5);

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

