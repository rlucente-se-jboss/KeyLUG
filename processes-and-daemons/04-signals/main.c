#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

void child_trap(int signum) {
	syslog(LOG_INFO, "Child exiting with signal %d", signum);
}

void parent_trap(int signum) {
	syslog(LOG_INFO, "Parent exiting with signal %d", signum);
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

		/* wait until SIGINT (CTRL-C) */
		signal(SIGINT, &parent_trap);
		sleep(1000);

                /* make the child an orphan */
		syslog(LOG_INFO, "Parent exiting ...");
                exit(EXIT_SUCCESS);
	}

	/* Child process gets here */
	report_pgs("Initial Child");

	/* wait for SIGINT */
	signal(SIGINT, &child_trap);
	sleep(1000);

        syslog(LOG_INFO, "Child exiting ...");
        return EXIT_SUCCESS;
}

