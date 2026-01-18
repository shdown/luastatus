This directory contains two test systems for luastatus:

  * pt (plain tests): the main test system; it tests plugins, barlibs and
    luastatus itself. It is written in bash.

  * torture: stress tests for luastatus, under valgrind.

kcov
----

kcov does not redirect the signals it receives to its child process.
This is why we have the kcov_wrapper thing: it is intended for wrapping
  kcov DIR COMMAND [ARGS...]
process. This wrapper spawns a process (normally kcov), then redirects the
following signals:
  * SIGINT;
  * SIGTERM;
  * SIGHUP;
  * SIGQUIT
to the child's child process. If the child spawns more than one process, then
which grandchild is selected for sending signals is unspecified.

Until the grandchild process is spawned, all signals received are queued.

It works under Linux only (requires procfs to be mounted at /proc).

Also, luastatus sometimes (rarely) segfaults when run under kcov.
We don't know why.
