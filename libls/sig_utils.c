#include "sig_utils.h"

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "errno_utils.h"

void
ls_xsigfillset(sigset_t *set)
{
    if (sigfillset(set) < 0) {
        LS_WITH_ERRSTR(s, errno,
            fprintf(stderr, "Unexpected error: sigfillset: %s\n", s);
        );
        abort();
    }
}

void
ls_xsigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0) {
        LS_WITH_ERRSTR(s, errno,
            fprintf(stderr, "Unexpected error: sigemptyset: %s\n", s);
        );
        abort();
    }
}
