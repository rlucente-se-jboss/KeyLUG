#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
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

