#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "libls/alloc_utils.h"
#include "libls/vector.h"
#include "libls/sprintf_utils.h"
#include "libls/osdep.h"

typedef LS_VECTOR_OF(char*) CstrVector;

static const char *IFS = " \t\n";

void
append_from_envvar_or(CstrVector *vec, const char *envvar, const char *const *def)
{
    bool appended = false;
    char *copy = NULL;

    const char *value = getenv(envvar);
    if (!value) {
        goto done;
    }
    copy = ls_xstrdup(value);
    for (char *token = strtok(copy, IFS); token; token = strtok(NULL, IFS)) {
        LS_VECTOR_PUSH(*vec, ls_xstrdup(token));
        appended = true;
    }

done:
    free(copy);
    if (!appended) {
        for (const char *const *s = def; *s; ++s) {
            LS_VECTOR_PUSH(*vec, ls_xstrdup(*s));
        }
    }
}

void
do_exec(CstrVector *argv)
{
    LS_VECTOR_PUSH(*argv, NULL);
    execvp(argv->data[0], argv->data);
    perror("execvp");
}

void
spawn_bidir_pipe(CstrVector *luastatus, const char *const *luastatus_args, CstrVector *lemonbar)
{
    int stdin_pipe[2] = {-1, -1};
    int stdout_pipe[2] = {-1, -1};
    pid_t pid = -1;

    if (pipe(stdin_pipe) < 0) {
        stdin_pipe[0] = stdin_pipe[1] = -1;
        perror("pipe");
        goto error;
    }
    if (pipe(stdout_pipe) < 0) {
        stdout_pipe[0] = stdout_pipe[1] = -1;
        perror("pipe");
        goto error;
    }
    if ((pid = fork()) < 0) {
        perror("fork");
        goto error;
    } else if (pid == 0) {
        if (dup2(stdin_pipe[0], 0) < 0 ||
            dup2(stdout_pipe[1], 1) < 0)
        {
            perror("dup2");
            goto child_error;
        }
        ls_close(stdin_pipe[0]);
        ls_close(stdin_pipe[1]);
        ls_close(stdout_pipe[0]);
        ls_close(stdout_pipe[1]);
        do_exec(lemonbar);
child_error:
        exit(127);
    } else {
        ls_close(stdin_pipe[0]);
        ls_close(stdout_pipe[1]);
        LS_VECTOR_PUSH(*luastatus, ls_xasprintf("-Bin_fd=%d", stdout_pipe[0]));
        LS_VECTOR_PUSH(*luastatus, ls_xasprintf("-Bout_fd=%d", stdin_pipe[1]));
        for (const char *const *s = luastatus_args; *s; ++s) {
            LS_VECTOR_PUSH(*luastatus, ls_xstrdup(*s));
        }
        do_exec(luastatus);
        goto error;
    }
error:
    exit(1);
}

int main(int argc, char **argv)
{
    CstrVector luastatus = LS_VECTOR_NEW();

    append_from_envvar_or(
        &luastatus,
        "LUASTATUS",
        (const char *const []) {"luastatus", NULL});

    LS_VECTOR_PUSH(luastatus, ls_xstrdup("-b"));

    append_from_envvar_or(
        &luastatus,
        "LUASTATUS_LEMONBAR_BARLIB",
        (const char *const []) {"lemonbar", NULL});

    CstrVector lemonbar = LS_VECTOR_NEW();
    append_from_envvar_or(
        &lemonbar,
        "LEMONBAR",
        (const char *const []) {"lemonbar", NULL});

    spawn_bidir_pipe(
        &luastatus,
        argc ? ((const char* const*) argv + 1) : (const char * const[]) {NULL},
        &lemonbar);
}
