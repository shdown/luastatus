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

#include "cmd_output.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "include/sayf_macros.h"
#include "libls/ls_panic.h"
#include "libls/ls_tls_ebuf.h"
#include "external_context.h"

static bool checked_pclose(ExternalContext ectx, FILE *f)
{
    int status = pclose(f);
    if (status < 0) {
        LS_FATALF(ectx, "[cmd_output] pclose: %s", ls_tls_strerror(errno));
        return false;
    }

    if (WIFEXITED(status)) {
        int rc = WEXITSTATUS(status);
        if (rc == 0) {
            return true;
        }
        LS_FATALF(ectx, "[cmd_output] process exited with non-zero code %d", rc);
        return false;

    } else if (WIFSIGNALED(status)) {
        int signum = WTERMSIG(status);
        LS_FATALF(ectx, "[cmd_output] process was killed with signal %d", signum);
        return false;

    } else {
        LS_FATALF(ectx, "[cmd_output] process terminated in unknown way (raw status: %d)", status);
        return false;
    }
}

static bool check_no_leftover(ExternalContext ectx, FILE *f)
{
    int c = fgetc(f);
    if (c == EOF) {
        return true;
    }

    LS_FATALF(ectx, "[cmd_output] process produced more than one line");
    LS_FATALF(ectx, "[cmd_output] will read its output until the end, then fail");

    do {
        c = fgetc(f);
    } while (c != EOF);

    return false;
}

char *cmd_output(ExternalContext ectx, const char *cmd)
{
    LS_ASSERT(cmd != NULL);

    FILE *f = popen(cmd, "r");
    if (!f) {
        LS_FATALF(ectx, "[cmd_output] popen: %s", ls_tls_strerror(errno));
        return NULL;
    }

    char *buf = NULL;
    size_t nbuf = 0;

    ssize_t r = getline(&buf, &nbuf, f);
    if (r <= 0) {
        if (ferror(f)) {
            LS_FATALF(ectx, "[cmd_output] getline: %s", ls_tls_strerror(errno));
        } else {
            LS_FATALF(ectx, "[cmd_output] getline: got EOF");
        }
        goto fail;
    }

    if (!check_no_leftover(ectx, f)) {
        goto fail;
    }

    if (!checked_pclose(ectx, f)) {
        f = NULL;
        goto fail;
    }

    if (buf[r - 1] == '\n') {
        buf[r - 1] = '\0';
    }
    return buf;

fail:
    if (f) {
        (void) pclose(f);
    }
    free(buf);
    return NULL;
}
