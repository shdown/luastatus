/*
 * Copyright (C) 2021-2025  luastatus developers
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

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>

static struct {
    bool half_duplex_ok;
    bool reuseaddr;
    bool print_line_when_ready;
    bool print_line_on_accept;
    bool just_check;
} global_flags;

static void fail(void)
{
    _exit(1);
}

static int full_write(int fd, const char *buf, size_t nbuf)
{
    for (size_t nwritten = 0; nwritten < nbuf;) {
        ssize_t w = write(fd, buf + nwritten, nbuf - nwritten);
        if (w < 0) {
            perror("parrot: write");
            return -1;
        }
        nwritten += w;
    }
    return 0;
}

typedef struct {
    int read_fd;
    int write_fd;
} ThreadArg;

static void *thread_func(void *varg)
{
    ThreadArg *arg = varg;
    int read_fd = arg->read_fd;
    int write_fd = arg->write_fd;

    char buf[1024];
    for (;;) {
        ssize_t r = read(read_fd, buf, sizeof(buf));
        if (r < 0) {
            perror("parrot: read");
            fail();
        }
        if (r == 0) {
            break;
        }
        if (full_write(write_fd, buf, r) < 0) {
            fail();
        }
    }

    if (!global_flags.half_duplex_ok) {
        _exit(0);
    }
    return NULL;
}

static void parrot(int other_fd)
{
    ThreadArg args[2] = {
        {.read_fd = other_fd, .write_fd = 1},
        {.read_fd = 0, .write_fd = other_fd},
    };
    pthread_t threads[2];
    for (int i = 0; i < 2; ++i) {
        if ((errno = pthread_create(&threads[i], NULL, thread_func, &args[i]))) {
            perror("parrot: pthead_create");
            fail();
        }
    }
    for (int i = 0; i < 2; ++i) {
        if ((errno = pthread_join(threads[i], NULL))) {
            perror("parrot: pthread_join");
            fail();
        }
    }
}

typedef struct {
    bool *out;
    const char *name;
} FlagOption;

static FlagOption flag_options[] = {
    {&global_flags.half_duplex_ok,        "--half-duplex-ok"},
    {&global_flags.reuseaddr,             "--reuseaddr"},
    {&global_flags.print_line_when_ready, "--print-line-when-ready"},
    {&global_flags.print_line_on_accept,  "--print-line-on-accept"},
    {&global_flags.just_check,            "--just-check"},
    {0},
};

static void print_usage(void)
{
    fprintf(stderr, "USAGE: parrot [options...] {TCP-CLIENT port | TCP-SERVER port | UNIX-CLIENT path | UNIX-SERVER path}\n");
    fprintf(stderr, "Supported options:\n");
    fprintf(stderr, "  --half-duplex-ok\n");
    fprintf(stderr, "    Continue even after one side has hung up.\n");
    fprintf(stderr, "  --reuseaddr\n");
    fprintf(stderr, "    Set SO_REUSEADDR option on the TCP server socket.\n");
    fprintf(stderr, "  --print-line-when-ready\n");
    fprintf(stderr, "    Write 'parrot: ready' line when the server is ready to accept a connection.\n");
    fprintf(stderr, "  --print-line-on-accept\n");
    fprintf(stderr, "    Write 'parrot: accepted' line when the server has accepted a connection.\n");
    fprintf(stderr, "  --just-check\n");
    fprintf(stderr, "    Exit with code 0 after creating the server.\n");
}

static int client(int (*func)(const char *arg), const char *arg)
{
    int fd = func(arg);
    if (fd < 0) {
        fail();
    }
    parrot(fd);
    close(fd);
    return 0;
}

static int server(int (*func)(const char *arg), const char *arg)
{
    int server_fd = func(arg);
    if (server_fd < 0) {
        fail();
    }

    if (global_flags.just_check) {
        close(server_fd);
        return 0;
    }

    if (global_flags.print_line_when_ready) {
        const char *line = "parrot: ready\n";
        if (full_write(1, line, strlen(line)) < 0) {
            fail();
        }
    }

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("parrot: accept");
        fail();
    }
    close(server_fd);

    if (global_flags.print_line_on_accept) {
        const char *line = "parrot: accepted\n";
        if (full_write(1, line, strlen(line)) < 0) {
            fail();
        }
    }

    parrot(client_fd);
    close(client_fd);
    return 0;
}

static in_addr_t localhost(void)
{
    in_addr_t res = inet_addr("127.0.0.1");
    if (res == (in_addr_t) -1) {
        perror("parrot: inet_addr");
        fail();
    }
    return res;
}

static int make_unix_client(const char *path)
{
    int fd = -1;

    struct sockaddr_un saun = {.sun_family = AF_UNIX};
    size_t npath = strlen(path);
    if (npath + 1 > sizeof(saun.sun_path)) {
        fprintf(stderr, "parrot: socket path is too long.\n");
        goto error;
    }
    memcpy(saun.sun_path, path, npath + 1);
    fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("parrot: socket");
        goto error;
    }
    if (connect(fd, (struct sockaddr *) &saun, sizeof(saun)) < 0) {
        perror("parrot: connect");
        goto error;
    }
    return fd;
error:
    close(fd);
    return -1;
}

static int make_tcp_client(const char *portstr)
{
    int fd = -1;

    unsigned port;
    if (sscanf(portstr, "%u", &port) != 1) {
        fprintf(stderr, "parrot: invalid port number: '%s'.\n", portstr);
        goto error;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("parrot: socket");
        goto error;
    }

    struct sockaddr_in sain = {
        .sin_family = AF_INET,
        .sin_addr = {.s_addr = localhost()},
        .sin_port = htons(port),
    };
    if (connect(fd, (struct sockaddr *) &sain, sizeof(sain)) < 0) {
        perror("parrot: connect");
        goto error;
    }
    return fd;

error:
    close(fd);
    return -1;
}

static int make_tcp_server(const char *portstr)
{
    int fd = -1;

    unsigned port;
    if (sscanf(portstr, "%u", &port) != 1) {
        fprintf(stderr, "parrot: invalid port number: '%s'.\n", portstr);
        goto error;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("parrot: socket");
        goto error;
    }

    if (global_flags.reuseaddr) {
        int value = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
            perror("parrot: setsockopt (SO_REUSEADDR)");
            goto error;
        }
    }

    struct sockaddr_in sain = {
        .sin_family = AF_INET,
        .sin_addr = {.s_addr = localhost()},
        .sin_port = htons(port),
    };
    if (bind(fd, (struct sockaddr *) &sain, sizeof(sain)) < 0) {
        perror("parrot: bind");
        goto error;
    }
    if (listen(fd, SOMAXCONN) < 0) {
        perror("parrot: listen");
        goto error;
    }
    return fd;

error:
    close(fd);
    return -1;
}

static int make_unix_server(const char *path)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("parrot: socket");
        goto error;
    }

    struct sockaddr_un saun = {.sun_family = AF_UNIX};
    size_t npath = strlen(path);
    if (npath + 1 > sizeof(saun.sun_path)) {
        fprintf(stderr, "parrot: socket path is too long.\n");
        goto error;
    }
    memcpy(saun.sun_path, path, npath + 1);

    if (bind(fd, (struct sockaddr *) &saun, sizeof(saun)) < 0) {
        perror("parrot: bind");
        goto error;
    }
    if (listen(fd, SOMAXCONN) < 0) {
        perror("parrot: listen");
        goto error;
    }
    return fd;

error:
    close(fd);
    return fd;
}

int main(int argc, char **argv)
{
    int argi = 1;
    for (; argi < argc; ++argi) {
        const char *arg = argv[argi];
        if (arg[0] != '-') {
            break;
        }
        bool *out = NULL;
        for (FlagOption *o = flag_options; o->out; ++o) {
            if (strcmp(arg, o->name) == 0) {
                out = o->out;
                break;
            }
        }
        if (!out) {
            fprintf(stderr, "Unknown option: '%s'.\n", arg);
            print_usage();
            return 2;
        }
        *out = true;
    }
    if (argc - argi != 2) {
        fprintf(stderr, "Expected exactly 2 positional arguments, found %d.\n", argc - argi);
        print_usage();
        return 2;
    }
    const char *what     = argv[argi];
    const char *what_arg = argv[argi + 1];

    if (strcmp(what, "TCP-CLIENT") == 0) {
        return client(make_tcp_client, what_arg);

    } else if (strcmp(what, "TCP-SERVER") == 0) {
        return server(make_tcp_server, what_arg);

    } else if (strcmp(what, "UNIX-CLIENT") == 0) {
        return client(make_unix_client, what_arg);

    } else if (strcmp(what, "UNIX-SERVER") == 0) {
        return server(make_unix_server, what_arg);

    } else {
        fprintf(stderr, "Unknown command: '%s'.\n", what);
        print_usage();
        return 2;
    }
}
