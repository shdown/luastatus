#include "pth_check.h"

#include <stdio.h>
#include <stdlib.h>
#include "libls/errno_utils.h"

void
pth_check__failed(int retval, const char *expr, const char *file, int line)
{
    LS_WITH_ERRSTR(s, retval,
        fprintf(stderr, "PTH_CHECK(%s) failed at %s:%d.\nReason: %s\n", expr, file, line, s);
    );
    abort();
}
