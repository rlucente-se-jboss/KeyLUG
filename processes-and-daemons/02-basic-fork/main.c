#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

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
		printf("Parent exiting ...\n");
		exit(EXIT_SUCCESS);
	}

	/* Child process gets here */
	report_pgs("Initial Child");

	/* wait a second for reaping by process 1 */
	sleep(1);
	report_pgs("Reaped Child");

	/* wait briefly before exiting */
	sleep(5);

	printf("Child exiting\n");
	return EXIT_SUCCESS;
}

