#ifndef ls_sig_utils_h_
#define ls_sig_utils_h_

#include <signal.h>

// Like /sigfillset/, but panics on failure.
void
ls_xsigfillset(sigset_t *set);

// Like /sigemptyset/, but panics on failure.
void
ls_xsigemptyset(sigset_t *set);

#endif
