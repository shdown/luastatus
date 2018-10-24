#ifndef ls_sig_utils_h_
#define ls_sig_utils_h_

#include <signal.h>
#include "panic.h"
#include "compdep.h"

// Like /sigfillset/, but panics on failure.
LS_INHEADER
void
ls_xsigfillset(sigset_t *set)
{
    if (sigfillset(set) < 0) {
        LS_PANIC("sigfillset() failed");
    }
}

// Like /sigemptyset/, but panics on failure.
LS_INHEADER
void
ls_xsigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0) {
        LS_PANIC("sigemptyset() failed");
    }
}

#endif
