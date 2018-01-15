The main problem is that all the stdio functions may, according to POSIX, fail with `EINTR`, and there is no way to restart some of them, e.g. `fprintf`.

Our solution is to ensure that `SA_RESTART` is set for all the signals that could be raised (without terminating the program).

See also: http://man7.org/linux/man-pages/man7/signal.7.html, section "Interruption of system calls and library functions by signal handlers".
