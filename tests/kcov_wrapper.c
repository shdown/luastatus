/*
 * Copyright (C) 2026  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

// This thing is intended for wrapping "kcov DIR COMMAND [ARGS...]" process: kcov
// does not redirect the signals it receives to its child process.
//
// This wrapper spawns a process (normally kcov), then redirects the following signals:
//   * SIGINT;
//   * SIGTERM;
//   * SIGHUP;
//   * SIGQUIT
// to the child's child process. If the child spawns more than one process, then which
// grandchild is selected for sending signals is unspecified.
//
// Until the grandchild process is spawned, all signals received are queued.
//
// It works under Linux only (requires procfs to be mounted at /proc).

#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <spawn.h>
#include <poll.h>

#define MY_NAME "kcov_wrapper"

static inline void say_perror(const char *msg)
{
    fprintf(stderr, "%s: %s: %s\n", MY_NAME, msg, strerror(errno));
}

static inline void say_error(const char *msg)
{
    fprintf(stderr, "%s: %s\n", MY_NAME, msg);
}

enum {
    RC_SPAWN_ERROR = 50,
    RC_UNEXPECTED_ERROR = 51,
    RC_CHILD_TERMINATED_IN_WEIRD_WAY = 99,
};

static int self_pipe_fds[2] = {-1, -1};

static int make_cloexec(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0) {
        return -1;
    }
    flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) < 0) {
        return -1;
    }
    return fd;
}

static bool make_self_pipe(void)
{
    if (pipe(self_pipe_fds) < 0) {
        say_perror("pipe");
        return false;
    }
    (void) make_cloexec(self_pipe_fds[0]);
    (void) make_cloexec(self_pipe_fds[1]);
    return true;
}

static int full_read(int fd, char *buf, size_t nbuf)
{
    size_t nread = 0;
    while (nread != nbuf) {
        ssize_t r = read(fd, buf + nread, nbuf - nread);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (r == 0) {
            return 0;
        } else {
            nread += r;
        }
    }
    return 1;
}

static int wait_for_input_on_fd(int fd, int timeout_ms)
{
    struct pollfd pfd = {.fd = fd, .events = POLLIN};
    int rc = poll(&pfd, 1, timeout_ms);
    if (rc < 0 && errno != EINTR) {
        say_perror("poll");
        abort();
    }
    return rc;
}

static int full_write(int fd, const char *data, size_t ndata)
{
    size_t nwritten = 0;
    while (nwritten != ndata) {
        ssize_t w = write(fd, data + nwritten, ndata - nwritten);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        nwritten += w;
    }
    return 0;
}

static void sig_handler(int signo)
{
    int saved_errno = errno;

    if (full_write(self_pipe_fds[1], (char *) &signo, sizeof(signo)) < 0) {
        abort();
    }

    errno = saved_errno;
}

static bool handle_signal(int signo)
{
    sigset_t empty;
    if (sigemptyset(&empty) < 0) {
        say_perror("sigemptyset");
        return false;
    }

    int flags = SA_RESTART;
    if (signo == SIGCHLD) {
        flags |= SA_NOCLDSTOP;
    }

    struct sigaction sa = {
        .sa_handler = sig_handler,
        .sa_mask = empty,
        .sa_flags = flags,
    };
    if (sigaction(signo, &sa, NULL) < 0) {
        say_perror("sigaction");
        return false;
    }

    return true;
}

static const int TERM_SIGNALS[] = {
    SIGINT,
    SIGTERM,
    SIGHUP,
    SIGQUIT,
};
enum { TERM_SIGNALS_NUM = sizeof(TERM_SIGNALS) / sizeof(TERM_SIGNALS[0]) };

static bool install_signal_handlers(void)
{
    if (!handle_signal(SIGCHLD)) {
        return false;
    }
    for (int i = 0; i < TERM_SIGNALS_NUM; ++i) {
        if (!handle_signal(TERM_SIGNALS[i])) {
            return false;
        }
    }
    return true;
}

static int wait_and_make_exit_code(pid_t pid)
{
    int status;
    pid_t rc;
    while ((rc = waitpid(pid, &status, 0)) < 0 && errno == EINTR) {
        // do nothing
    }
    if (rc < 0) {
        say_perror("waitpid");
        return RC_UNEXPECTED_ERROR;
    } else if (rc == 0) {
        say_error("waitpid returned 0");
        return RC_UNEXPECTED_ERROR;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return RC_CHILD_TERMINATED_IN_WEIRD_WAY;
}

static pid_t get_grandchild_pid(pid_t child_pid)
{
    static const char *SHELL_PROGRAM_SUFFIX =
        "cd /proc || exit $?\n"
        "exec 2>/dev/null || exit $?\n"
        "for pid in [0-9]*; do\n"
        "  read _ _ _ ppid _ < $pid/stat || continue\n"
        "  if [ $ppid = $target_pid ]; then\n"
        "    x=$(readlink $pid/exe) || continue\n"
        "    case \"$x\" in\n"
        "    */luastatus) echo $pid; exit $? ;;\n"
        "    esac"
        "  fi\n"
        "done\n"
        "echo 0\n"
    ;

    enum { CAPACITY = 16 * 1024 };
    char *prog = malloc(CAPACITY);
    if (!prog) {
        say_perror("malloc");
        abort();
    }
    int nprog = snprintf(prog, CAPACITY, "target_pid=%jd; %s", (intmax_t) child_pid, SHELL_PROGRAM_SUFFIX);
    if (nprog < 0 || nprog > CAPACITY - 32) {
        say_error("snprintf failed (shell program)");
        abort();
    }

    pid_t res = -1;

    FILE *f = popen(prog, "r");
    if (!f) {
        say_perror("popen");
        goto cleanup;
    }
    intmax_t raw_res;
    if (fscanf(f, "%jd\n", &raw_res) != 1) {
        say_error("fscanf failed (from shell program)");
        goto cleanup;
    }
    res = raw_res;

cleanup:
    if (f) {
        int status = pclose(f);
        if (status < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            say_error("our shell program failed");
            res = -1;
        }
    }
    free(prog);
    return res;
}

typedef struct {
    pid_t child_pid;
    pid_t grandchild_pid;
} State;

static bool state_has_grandchild_pid(State *s)
{
    if (s->grandchild_pid) {
        return true;
    }
    pid_t new_pid = get_grandchild_pid(s->child_pid);
    if (new_pid < 0) {
        say_error("(warning) cannot get real luastatus' PID");
        return false;
    } else if (new_pid == 0) {
        say_error("(warning) real luastatus was not spawned yet");
        return false;
    } else {
        fprintf(stderr, "%s: info: real luastatus' PID = %jd\n", MY_NAME, (intmax_t) new_pid);
        s->grandchild_pid = new_pid;
        return true;
    }
}

static void state_kill(State *s, int signo)
{
    fprintf(stderr, "%s: info: killing luastatus with signal %d\n", MY_NAME, signo);
    if (kill(s->grandchild_pid, signo) < 0) {
        say_perror("(warning) kill");
    }
}

typedef struct {
    int signums[TERM_SIGNALS_NUM];
} QueuedSignals;

#define queued_signals_nonempty(Q_) (((Q_)->signums[0]) != 0)

#define queued_signals_at(Q_, Idx_) ((Q_)->signums[(Idx_)])

static inline void queued_signals_add(QueuedSignals *q, int signo)
{
    bool found = false;
    for (int i = 0; i < TERM_SIGNALS_NUM; ++i) {
        found |= (TERM_SIGNALS[i] == signo);
    }
    if (!found) {
        return;
    }
    for (int i = 0; i < TERM_SIGNALS_NUM; ++i) {
        if (q->signums[i] == signo) {
            return;
        } else if (!q->signums[i]) {
            q->signums[i] = signo;
            return;
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s COMMAND [ARGS...]\n", MY_NAME);
        return 2;
    }

    char **child_argv = argv + 1;

    if (!make_self_pipe()) {
        return RC_UNEXPECTED_ERROR;
    }
    if (!install_signal_handlers()) {
        return RC_UNEXPECTED_ERROR;
    }

    extern char **environ;

    pid_t child_pid;
    int rc = posix_spawnp(
        &child_pid,
        child_argv[0],
        NULL,
        NULL,
        child_argv,
        environ);

    if (rc != 0) {
        errno = rc;
        say_perror("posix_spawnp");
        return RC_SPAWN_ERROR;
    }

    fprintf(stderr, "%s: info: child PID: %jd\n", MY_NAME, (intmax_t) child_pid);

    State state = {.child_pid = child_pid, .grandchild_pid = 0};

    QueuedSignals q = {0};

    for (;;) {

        int poll_rc = wait_for_input_on_fd(self_pipe_fds[0], /*timeout_ms=*/ 200);
        if (poll_rc == 0) {
            // Timeout
            if (queued_signals_nonempty(&q)) {
                say_error("timeout and we have queued signals");
                if (state_has_grandchild_pid(&state)) {
                    for (int i = 0; i < TERM_SIGNALS_NUM; ++i) {
                        int signo = queued_signals_at(&q, i);
                        if (signo) {
                            state_kill(&state, signo);
                        }
                    }
                }
                q = (QueuedSignals) {0};
            }
            continue;
        }

        int signo;
        int read_rc = full_read(self_pipe_fds[0], (char *) &signo, sizeof(signo));
        if (read_rc < 0) {
            say_perror("read (from self-pipe)");
            return RC_UNEXPECTED_ERROR;
        } else if (read_rc == 0) {
            say_error("read (from self-pipe) returned 0");
            return RC_UNEXPECTED_ERROR;
        }

        if (signo == SIGCHLD) {
            fprintf(stderr, "%s: info: got SIGCHLD\n", MY_NAME);
            return wait_and_make_exit_code(child_pid);
        } else {
            fprintf(stderr, "%s: info: got signal %d\n", MY_NAME, signo);

            if (state_has_grandchild_pid(&state)) {
                state_kill(&state, signo);
            } else {
                say_error("queueing this signal");
                queued_signals_add(&q, signo);
            }
        }
    }
}
