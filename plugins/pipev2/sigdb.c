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

#include "sigdb.h"
#include <stddef.h>
#include <string.h>
#include <signal.h>
#include "libls/ls_panic.h"

typedef struct {
    const char *sig_name;
    int sig_num;
} SigSpec;

static const SigSpec SIG_SPECS[] = {
    {"SIGABRT", SIGABRT},
    {"SIGALRM", SIGALRM},
    {"SIGBUS",  SIGBUS},
    {"SIGCHLD", SIGCHLD},
    {"SIGCONT", SIGCONT},
    {"SIGFPE",  SIGFPE},
    {"SIGHUP",  SIGHUP},
    {"SIGILL",  SIGILL},
    {"SIGINT",  SIGINT},
    {"SIGKILL", SIGKILL},
    {"SIGPIPE", SIGPIPE},
    {"SIGQUIT", SIGQUIT},
    {"SIGSEGV", SIGSEGV},
    {"SIGSTOP", SIGSTOP},
    {"SIGTERM", SIGTERM},
    {"SIGTSTP", SIGTSTP},
    {"SIGTTIN", SIGTTIN},
    {"SIGTTOU", SIGTTOU},
    {"SIGUSR1", SIGUSR1},
    {"SIGUSR2", SIGUSR2},
    {"SIGURG",  SIGURG},
    {0},
};

int sigdb_lookup_num_by_name(const char *sig_name)
{
    LS_ASSERT(sig_name != NULL);

    for (const SigSpec *p = SIG_SPECS; p->sig_name; ++p) {
        if (strcmp(sig_name, p->sig_name) == 0) {
            return p->sig_num;
        }
    }
    return -1;
}
