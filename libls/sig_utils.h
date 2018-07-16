#ifndef ls_sig_utils_h_
#define ls_sig_utils_h_

#include <signal.h>

void
ls_xsigfillset(sigset_t *set);

void
ls_xsigemptyset(sigset_t *set);

#endif
