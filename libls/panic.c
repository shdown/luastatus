#include "panic.h"

#include "cstring_utils.h"

void
ls_pth_check_impl(int ret, const char *expr, const char *file, int line)
{
    if (ret == 0) {
        return;
    }
    fprintf(stderr, "LS_PTH_CHECK(%s) failed at %s:%d, reason: %s\nAborting.\n",
            expr, file, line, ls_strerror_onstack(ret));
    abort();
}
