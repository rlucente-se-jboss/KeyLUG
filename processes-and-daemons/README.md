# processes and daemons
These projects can be used to explore processes and daemons in
Linux.  All of these examples run on RHEL 8.x.  Developers can get
RHEL 8 for free at https://developers.redhat.com

## 01-basic-pgs
Report a process' id, parent process id, process group, and session.

To build and run,

    git clone https://github.com/rlucente-se-jboss/KeyLUG.git
    cd KeyLUG/processes-and-daemons/01-basic-pgs
    cmake .
    make
    ps -f && ./simple-daemon && echo

## 02-basic-fork

    cd ../02-basic-fork
    cmake .
    make
    ps -f && ./simple-daemon && sleep 2 && ps -f && sleep 5 && echo

## 03-syslog

    cd ../03-syslog
    cmake .
    make

In separate terminal,

    journalctl -af

In original terminal,

    ./simple-daemon

## 04-signals

In a separate terminal,

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

Finally, manage jobs using fg by

    ./simple-daemon &
    fg

Then CTRL-C to force teh parent and child to exit.

## 05-non-systemd-example
In a separate terminal,

    journalctl -af

To build and run,

    cd ../05-non-systemd-example
    cmake .
    make
    ./simple-daemon

Get the child pid from the journalctl window, and use it to exame
the process

    less /proc/<PID>/status

Then look at the UMask.  Next,

    lsof -p <PID>

and look at the open file descriptors and default working directory.
Finally, kill the process using

    kill <PID>

## 06-systemd-example
In a separate terminal,

    journalctl -af

To build and run,

    cd ../06-systemd-example
    cmake .
    make
    sudo make install

    sudo systemctl enable simple-daemon
    sudo systemctl start simple-daemon

Get the child pid from the journalctl window, and use it to exame
the process

    less /proc/<PID>/status

Then look at the UMask.  Next,

    lsof -p <PID>

and look at the open file descriptors and default working directory.
Finally, kill the process using

    kill <PID>

Systemd automatically restarts the daemon.  To really stop it, use

    sudo systemctl stop simple-daemon

Reboot the system and then observe that the daemon automatically
restarts.

