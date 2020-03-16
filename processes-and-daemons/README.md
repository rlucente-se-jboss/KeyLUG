# processes and daemons
These projects can be used to explore processes and daemons in
Linux.  All of these examples run on RHEL 8.x.  Developers can get
RHEL 8 for free at https://developers.redhat.com

## Review Key Concepts
In a terminal, type the following command:

    man 7 credentials

If the man page isn't available, install the man packages:

    sudo yum -y install man-pages man-pages-overrides

Review the initial topics of process, parent process, process groups,
sessions, and session leaders.  Make sure you understand what a
controlling terminal is.

## 01-basic-pgs
Report a process' id, parent process id, process group, and session.

To build and run,

    git clone https://github.com/rlucente-se-jboss/KeyLUG.git
    cd KeyLUG/processes-and-daemons/01-basic-pgs
    cmake .
    make
    ps -f && ./simple-daemon && echo

## 02-basic-fork
Processes are principally created via the fork() system call although
there's a newer system call clone() that we won't cover here.  Review
the fork man page,

    man 2 fork

Next, let's explore some source code that uses fork,

    cd ../02-basic-fork
    cmake .
    make
    ps -f && ./simple-daemon && sleep 2 && ps -f && sleep 5 && echo

## 03-syslog
Syslog is a great facility where administrators can control the
destination of log information while developers focus on capturing
key events in the application.  Explore a small program that uses
syslog,

    cd ../03-syslog
    cmake .
    make

In separate terminal,

    journalctl -af

In original terminal,

    ./simple-daemon

## 04-signals
Review the topic of signals by reviewing the primary man page,

    man 7 signal

Next, let's explore some signal processing. In a separate terminal,

    journalctl -af

In original terminal,

    cmake .
    make
    ./simple-daemon

then CTRL-C after a few seconds.  Both parent and child exit on the
signal.

    ./simple-daemon &

Identify the PID output by the shell, and then

    kill -2 <PID>

Only that process will exit.  The other process (either parent or
child) will need to be killed separately.

If you kill the child first, you'll create a zombie process.  It
appears as 'defunct' when listing processes...

    ps -ef | grep simple-daemon

Send any signal you want to the defunct process.  To really kill
it, the parent needs to exit.  You can avoid this by having the
parent wait for status from the child or if the parent exits so the
child is reaped by PID 1 (on RHEL 8, this is systemd).

Finally, manage jobs using fg by

    ./simple-daemon &
    fg

Then CTRL-C to force the parent and child to exit.

## 05-non-systemd-example
This is a complete example that fleshes out all the detail necessary
to run a SysV daemon on Linux.  Review the relevant man page here,

    man 7 daemon

All listed items in the man page are highlighted in the source code.

In a separate terminal,

    journalctl -af

To build,

    cd ../05-non-systemd-example
    cmake .
    make

Get usage help,

    ./simple-daemon --help

Run as non-daemon,

    ./simple-daemon

CTRL-C will exit the program.

Run as daemon in local directory as current user,

    ./simple-daemon -d -l my.lock -p my.pid

Run as daemon and drop privileges to user invoking sudo,

    sudo ./simple-daemon -d

Get the daemon pid from the journalctl window, and use it to examine
the process

    less /proc/<PID>/status

Then look at the `UMask` line near the top.  Next,

    sudo lsof -p <PID>

and look at the open file descriptors and default working directory.
Finally, kill the process using

    kill <PID>

## 06-systemd-example
Review the daemon man page for how to run new-style daemons using
systemd.

    man 7 daemon

TODO: This example can use some cleanup to fully comply with those
instructions.

In a separate terminal,

    journalctl -af

To build and run,

    cd ../06-systemd-example
    cmake .
    make
    sudo make install

    sudo systemctl enable simple-daemon
    sudo systemctl start simple-daemon

Get the child pid from the journalctl window, and use it to examinee
the process

    less /proc/<PID>/status

Then look at the `UMask`.  Next,

    lsof -p <PID>

and look at the open file descriptors and default working directory.
Finally, kill the process using

    kill <PID>

Systemd automatically restarts the daemon no matter what signal you
send.  To really stop it, use

    sudo systemctl stop simple-daemon

Reboot the system and then observe that the daemon automatically
restarts.

