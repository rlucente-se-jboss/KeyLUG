#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

extern char **environ;

/*
 * Flag to keep daemon running. Set to zero on receive of SIGTERM
 */
static int running = 1;

/*
 * Minimal environment variable list. Some system calls require PATH
 */
static char *limited_env[] = { "PATH=" _PATH_STDPATH, 0 };

/*
 * Lock file descriptor.  Held open until process exits so that only one daemon
 * at a time runs.
 */
static int lock_fd;

/*
 * Close all open file descriptors except standard input, output, and error
 * (i.e. the first three file descriptors 0, 1, 2). This ensures that no
 * accidentally passed file descriptor stays around in the daemon process. On
 * Linux, this is best implemented by iterating through /proc/self/fd, with a
 * fallback of iterating from file descriptor 3 to the value returned by
 * getrlimit() for RLIMIT_NOFILE.
 */
static void close_all_fds() {
    DIR *fd_list;

    fd_list = opendir("/proc/self/fd");
    if (fd_list) {	               	    // successfully opened the directory
        int dir_fd = dirfd(fd_list);    // save the fd for /proc/self/fd
        struct dirent *current_entry;   // current entry in the directory

        const char *filename;
        char *end_filename;
        while ((current_entry = readdir(fd_list)) != NULL) {
            filename = current_entry->d_name;

            /* current filename to base 10 number, but check if failed */
            int current_fd = strtol(filename, &end_filename, 10);
            if (   *filename != '\0'
                && *end_filename == '\0' // we converted an actual number
                && current_fd != dir_fd	 // current isn't the directory itself
                && current_fd > 2)		 // current isn't stdin, stdout, stderr
                close(current_fd);
        }
    } else {
        /*
         * if no /proc/self/fd determine soft limit for open file descriptors
         */
        struct rlimit fd_limit;
        if (getrlimit(RLIMIT_NOFILE, &fd_limit) == -1)
            die(__LINE__, "failed to get number of files");
        /*
         * close all open files except stdin, stdout, stderr.  This is
         * overkill, but works
         */
        for (int i = 3; i < fd_limit.rlim_cur; i++)
            close (i);
    }
}

/*
 * Acquire advisory lock to ensure only one daemon process is running. Die if
 * unable. All locks are released on program exit when open file descriptors
 * are closed. This uses open file description locks as described in
 * "man 2 fcntl".
 */
static void check_if_running(char *lock_filename) {
    struct flock fl;

    lock_fd = open(lock_filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (lock_fd == -1)
        die(__LINE__, "Unable to open lock file");

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = 0;

    if (fcntl(lock_fd, F_OFD_SETLK, &fl) == -1)
        die(__LINE__, "lock failed due to possible other instance running");
}

/*
 * Sanitize the environment block, removing or resetting environment variables
 * that might negatively impact daemon runtime.  This function will only
 * include a default PATH variable but you can expand this function to include
 * other environment variables that your daemon needs.
 */
static void sanitize_env() {
    char *env_val, *cur_val, **new_environ;
    size_t env_count = 1, new_block_size = 0, len;

    /*
     * get the size of each env variable including the terminating null and
     * count the environment variables
     */
    while ((env_val = limited_env[env_count - 1]) != 0) {
        new_block_size += strlen(env_val) + 1;
        env_count++;
    }

    /*
     * allocate one memory block that holds both the pointer table and the env
     * vars. If we can't allocate memory, then die
     */
    new_block_size += (env_count * sizeof(char *));
    if (!(new_environ = (char **)malloc(new_block_size)))
        die(__LINE__, "failed to allocate memory for new environment");

    /* populate new environment block with pointers and env vars */
    new_environ[env_count - 1] = 0;

    cur_val = (char *)new_environ + (env_count * sizeof(char *));
    for (int i = 0; (env_val = limited_env[i]) != 0; i++) {
        new_environ[i] = cur_val;
        env_val = limited_env[i];
        len = strlen(env_val) + 1;
        memcpy(cur_val, env_val, len);
        cur_val += len;
    }

    /* replace with new sanitized environment variables */
    environ = new_environ;
}

/*
 * Reset all signal handlers to their default. This is best done by iterating
 * through the available signals up to the limit of _NSIG and resetting them
 * to SIG_DFL.
 *
 * Reset the signal mask using sigprocmask().
 */
static void reset_all_signal_handlers() {
    for (int i = 1; i < _NSIG; i++)
        signal(i, SIG_DFL);

    /* unblock all signals */
    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigprocmask(SIG_SETMASK, &signal_set, NULL);
}

/*
 * This complies with "man 7 daemon" for SysV daemons
 */
static void make_daemon(char *lock_filename, char *pid_filename,
    char *target_user) {
    /*
     * Item 1
     * Close all open file descriptors except standard input, output, and error
     * (i.e. the first three file descriptors 0, 1, 2). This ensures that no
     * accidentally passed file descriptor stays around in the daemon process.
     * On Linux, this is best implemented by iterating through /proc/self/fd,
     * with a fallback of iterating from file descriptor 3 to the value
     * returned by getrlimit() for RLIMIT_NOFILE.
     */
    close_all_fds();

    /*
     * Item 12 check for exclusivity
     * Make sure that the daemon PID can be written to a PID file to ensure
     * that the daemon cannot be started more than once. This must be
     * implemented in race-free fashion so that the PID file is only updated
     * when it is verified at the same time that the PID previously stored in
     * the PID file no longer exists or belongs to a foreign process.
     */
    check_if_running(lock_filename);

    /*
     * Items 2 and 3
     * Reset all signal handlers to their default. This is best done by
     * iterating through the available signals up to the limit of _NSIG and
     * resetting them to SIG_DFL.
     *
     * Reset the signal mask using sigprocmask().
     */
    reset_all_signal_handlers();

    /*
     * Item 4
     * Sanitize the environment block, removing or resetting environment
     * variables that might negatively impact daemon runtime.
     */
    sanitize_env();

    /*
     * Item 14
     * Open an unnamed pipe for the child to communicate start up status to the
     * parent. pipefd[0] is the read end and pipefd[1] is the write end
     */
    int pipefd[2];
    if (pipe(pipefd) == -1)
        die(__LINE__, "failed to open pipe");

    /*
     * Item 5
     * Call fork(), to create a background process.
     */
    pid_t pid = fork();
    if (pid == -1)
        die(__LINE__, "parent failed to fork");
    else if (pid > 0) {
        /*
         * We're the parent process
         */
        report_pgs("Parent");

        /*
         * close the write end of the pipe
         */
        close(pipefd[1]);

        /*
         * wait for status from the daemon process
         */
        char result[3];
        memset(result, 0, 3);

        if (read(pipefd[0], &result, 3) == -1)
            die(__LINE__, "unable to read the pipe");

        /*
         * close read end of pipe now
         */
        close(pipefd[0]);

        /*
         * Item 15
         * Call exit() in the original process. The process that invoked the
         * daemon must be able to rely on that this exit() happens after
         * initialization is complete and all external communication channels
         * are established and accessible.
         */
        if (result[0] == '0')
            exit(EXIT_SUCCESS);
        else
            die(__LINE__, "daemon failed at %s", result);
    } else {
        /*
         * We're the first child process
         */
        sleep(1);
        report_pgs("Child");
         
        /*
         * close the read end of the pipe
         */
        close(pipefd[0]);
  
        /*
         * Item 6
         * Detach from any terminal and create an independent session
         */
        if (setsid() == -1) {
            write(pipefd[1], "1", 2);
            exit(EXIT_FAILURE);
        }

        /*
         * Item 7
         * Call fork() again to ensure that the daemon can never re-acquire a
         * terminal again
         */
        pid = fork();
        if (pid == -1) {
            write(pipefd[1], "2", 2);
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            /*
             * We're the original child
             */

            /*
             * Item 8
             * First child exits so that only the second child (the actual
             * daemon process) stays around.  This ensures that the daemon
             * process is re-parented to PID 1, as all daemons should be.
             */
            exit(EXIT_SUCCESS);
        } else {
            /*
             * The daemon process
             */
            sleep(1);
            report_pgs("Daemon");

            /*
             * Item 9
             * Connect /dev/null to standard input, output, and error
             */

            for (int i = 0; i < 3; i++)
                close(i);

            if (open("/dev/null", O_RDWR) == -1) {
                write(pipefd[1], "3", 2);
                exit(EXIT_FAILURE);
            }

            if (dup(0) == -1) {
                write(pipefd[1], "4", 2);
                exit(EXIT_FAILURE);
            }

            if (dup(0) == -1) {
                write(pipefd[1], "5", 2);
                exit(EXIT_FAILURE);
            }
            
            /*
             * Item 10
             * Reset the umask to 0, so that the file modes passed to open(),
             * mkdir() and suchlike directly control the access mode of the
             * created files and directories.
             */
            umask(0);

            /*
             * Item 11
             * Change the current directory to the root directory (/), in order
             * to avoid that the daemon involuntarily blocks the mount points
             * from being unmounted.
             */
            if (chdir ("/") == -1) {
                write(pipefd[1], "6", 2);
                exit(EXIT_FAILURE);
            }

            /*
             * Item 12
             * Write the daemon PID (as returned by getpid()) to a PID file,
             * for example /run/foobar.pid (for a hypothetical daemon "foobar")
             * to ensure that the daemon cannot  be started more than once.
             */
            FILE *pid_file = NULL;

            pid_file = fopen(pid_filename, "w");
            if (pid_file == NULL) {
                write(pipefd[1], "7", 2);
                exit(EXIT_FAILURE);
            }

            int rc = fprintf(pid_file, "%d\n", getpid());
            if (rc < 0) {
                write(pipefd[1], "8", 2);
                exit(EXIT_FAILURE);
            }

            fclose(pid_file);

            /*
             * Item 13
             * Drop privileges, if possible and applicable
             */
            if (target_user) {
                struct passwd *pwd_entry;
                
                pwd_entry = getpwnam(target_user);
                if (pwd_entry == NULL) {
                    write(pipefd[1], "9", 2);
                    exit(EXIT_FAILURE);
                }

                if (setuid(pwd_entry->pw_uid) == -1) {
                    write(pipefd[1], "10", 3);
                    exit(EXIT_FAILURE);
                }
            }

            /*
             * Item 14
             * notify the original parent that initialization is complete.
             */
            write(pipefd[1], "0", 2);
            close(pipefd[1]);
        }
    }
}

/*
 * Signal handler to shutdown on receipt of SIGTERM
 */
void handle_signal(int signum) {
    log_info("Caught signal %d", signum);
    if (signum == SIGTERM || signum == SIGINT) {
        log_info("Exiting on signal");
        running = 0;
    }
    
    /*
     * could possibly reload configuration on SIGHUP
     */
}

void usage(char **argv) {
    printf("Usage: %s [OPTIONS]\n\n", argv[0]);
    printf("  -d, --daemon      Run process as a SysV-style daemon\n");
    printf("  -l, --lockfile    File to ensure only one daemon as a time\n");
    printf("                    Default is /run/lock/%s.lock\n", argv[0]);
    printf("  -p, --pidfile     File to save the daemon pid\n");
    printf("                    Default is /run/%s.pid\n", argv[0]);
    printf("  -u, --user        User name for daemon to run as\n");
    printf("                    Default is $SUDO_USER, otherwise $USER\n",
        argv[0]);
    printf("  -h, --help        These usage instructions\n\n");
}

int main (int argc, char **argv)
{
    const char *const default_pid_dir = "/run/";
    const char *const default_lock_dir = "/run/lock/";

    char pidfile[PATH_MAX];
    char lockfile[PATH_MAX];
    char *user    = 0;
    int  daemon_mode = 0;

    /*
     * set reasonable defaults for arguments for when daemon_mode is true
     *
     * pidfile is /run/argv[0].pid
     * lockfile is /run/lock/argv[0].lock
     * user is either user who invoked sudo or actual user
     */
    memset(pidfile, 0, PATH_MAX);
    memset(lockfile, 0, PATH_MAX);

    sprintf(pidfile, "%s%s.pid", default_pid_dir, argv[0]);
    sprintf(lockfile, "%s%s.lock", default_lock_dir, argv[0]);

    /* get user who invoked "sudo" if available */
    user = getenv("SUDO_USER");
    if (user == NULL)
        user = getenv("USER");

    /*
     * parse command line arguments. Running as a daemon is optional so we can
     * run the code in a shell.
     */
    static struct option long_options[] = {
        {"daemon",   no_argument,       0, 'd'},
        {"lockfile", required_argument, 0, 'l'},
        {"pidfile",  required_argument, 0, 'p'},
        {"user",     required_argument, 0, 'u'},
        {"help",     no_argument,       0, 'h'},
        {0,          0,                 0,  0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "dl:p:u:h", long_options, 0);
        if (c == -1)
            break;

        switch (c) {
        case 'd' :
            daemon_mode = 1;
            break;

        case 'l' :
            free(lockfile);
            strcpy(lockfile, optarg);
            break;

        case 'p' :
            free(pidfile);
            strcpy(pidfile, optarg);
            break;

        case 'u' :
            user = optarg;
            break;

        case 'h' :
        default:
            usage(argv);
            exit(0);
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Unexpected arguments:");
        for (; optind < argc; ++optind) fprintf(stderr, " %s", argv[optind]);
        fprintf(stderr, "\n\n");
        usage(argv);
        exit(1);
    }

    /*
     * make sure full path for filenames since daemon will cd to /
     */
    char actual_lockfile[PATH_MAX];
    char actual_pidfile[PATH_MAX];

    realpath(lockfile, actual_lockfile);
    realpath(pidfile, actual_pidfile);

    /*
     * daemonize the process
     */
    if (daemon_mode)
        make_daemon(actual_lockfile, actual_pidfile, user);

    /*
     * set handler for SIGHUP, SIGINT, and SIGTERM
     */
    signal(SIGHUP, handle_signal);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    /*
     * do its daemon thing...
     */
    log_info("Now running");

    int i = 0;
    while (running == 1) {
        log_info(((i++ % 2) == 0 ? "tick" : "tock"));
        sleep(2);
    }

    log_info("Exiting");
    return EXIT_SUCCESS;
}
