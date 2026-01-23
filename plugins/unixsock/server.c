#include "server.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include "libls/ls_alloc_utils.h"
#include "libls/ls_assert.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_time_utils.h"
#include "libls/ls_string.h"

#include "cloexec_accept.h"

enum { NCHUNK = 1024 };

typedef struct {
    int fd;
    LS_String buf;
} Client;

struct Server {
    int srv_fd;
    size_t max_clients;

    size_t nclients;
    struct pollfd *pfds;
    Client *clients;
};

Server *server_new(
        int fd,
        size_t max_clients)
{
    LS_ASSERT(fd >= 0);
    LS_ASSERT(max_clients <= SERVER_MAX_CLIENTS_LIMIT);

    ls_make_nonblock(fd);

    Server *S = LS_XNEW(Server, 1);
    *S = (Server) {
        .srv_fd = fd,
        .max_clients = max_clients,
        .nclients = 0,
        .pfds = LS_XNEW(struct pollfd, 1),
        .clients = NULL,
    };
    return S;
}

static inline bool is_full(Server *S)
{
    LS_ASSERT(S->nclients <= S->max_clients);

    return S->nclients == S->max_clients;
}

int server_poll(
        Server *S,
        LS_TimeDelta timeout,
        struct pollfd **out,
        size_t *out_n,
        bool *out_can_accept)
{
    struct pollfd *pfds = S->pfds;

    pfds[0] = (struct pollfd) {
        .fd = is_full(S) ? -1 : S->srv_fd,
        .events = POLLIN,
    };

    int rc = ls_poll(pfds, S->nclients + 1, timeout);
    if (rc < 0) {
        return -1;
    }

    *out = pfds + 1;
    *out_n = S->nclients;
    *out_can_accept = (pfds[0].revents != 0);
    return rc == 0 ? 0 : 1;
}

static inline bool is_dropped(Client *c)
{
    return c->fd < 0;
}

static void drop_client(Client *c)
{
    LS_ASSERT(!is_dropped(c));

    close(c->fd);
    c->fd = -1;

    ls_string_free(c->buf);
}

static inline bool find_newline(Client *c, size_t num_last_read)
{
    LS_ASSERT(num_last_read != 0);
    LS_ASSERT(num_last_read <= c->buf.size);

    const char *new_chunk = c->buf.data + c->buf.size - num_last_read;
    return memchr(new_chunk, '\n', num_last_read) != NULL;
}

int server_read_from_client(
        Server *S,
        size_t idx)
{
    LS_ASSERT(idx < S->nclients);
    Client *c = &S->clients[idx];
    struct pollfd *pfd = &S->pfds[idx + 1];

    LS_ASSERT(!is_dropped(c));

    if (!pfd->revents) {
        return 0;
    }

    ls_string_ensure_avail(&c->buf, NCHUNK);
    ssize_t r = read(c->fd, c->buf.data + c->buf.size, NCHUNK);
    if (r < 0) {
        if (LS_IS_EAGAIN(errno)) {
            return 0;
        }
        return -1;

    } else if (r == 0) {
        errno = 0;
        return -1;
    }

    c->buf.size += r;

    if (find_newline(c, r)) {
        return 1;
    }
    return 0;
}

const char *server_get_full_line(
        Server *S,
        size_t idx,
        size_t *out_len)
{
    LS_ASSERT(idx < S->nclients);

    Client *c = &S->clients[idx];

    const char *data = c->buf.data;
    size_t ndata = c->buf.size;

    LS_ASSERT(ndata != 0);

    const char *newline = memchr(data, '\n', ndata);
    LS_ASSERT(newline != NULL);

    *out_len = newline - data;
    return data;
}

void server_drop_client(
        Server *S,
        size_t idx)
{
    LS_ASSERT(idx < S->nclients);

    Client *c = &S->clients[idx];
    drop_client(c);
}

static void set_nclients(Server *S, size_t new_n)
{
    if (S->nclients == new_n) {
        return;
    }
    S->clients = LS_M_XREALLOC(S->clients, new_n);
    S->pfds = LS_M_XREALLOC(S->pfds, new_n + 1);
    S->nclients = new_n;
}

static void update_pfds(Server *S)
{
    for (size_t i = 0; i < S->nclients; ++i) {
        Client *c = &S->clients[i];
        LS_ASSERT(!is_dropped(c));

        S->pfds[i + 1] = (struct pollfd) {
            .fd = c->fd,
            .events = POLLIN,
        };
    }
}

static void add_client(Server *S, int fd)
{
    set_nclients(S, S->nclients + 1);
    S->clients[S->nclients - 1] = (Client) {
        .fd = fd,
        .buf = ls_string_new_reserve(NCHUNK),
    };
    update_pfds(S);
}

int server_accept_new_client(Server *S)
{
    LS_ASSERT(S->pfds[0].revents != 0);

    int client_fd = cloexec_accept(S->srv_fd);
    if (client_fd < 0) {
        if (LS_IS_EAGAIN(errno) || errno == ECONNABORTED) {
            return 0;
        }
        return -1;
    }

    ls_make_nonblock(client_fd);

    add_client(S, client_fd);
    return 1;
}

void server_compact(Server *S)
{
    size_t j = 0;
    for (size_t i = 0; i < S->nclients; ++i) {
        if (!is_dropped(&S->clients[i])) {
            S->clients[j] = S->clients[i];
            ++j;
        }
    }

    set_nclients(S, j);
    update_pfds(S);
}

void server_destroy(Server *S)
{
    close(S->srv_fd);

    for (size_t i = 0; i < S->nclients; ++i) {
        if (!is_dropped(&S->clients[i])) {
            drop_client(&S->clients[i]);
        }
    }

    free(S->clients);
    free(S->pfds);
    free(S);
}
