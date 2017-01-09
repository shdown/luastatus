#include "alloc_utils.h"

#include <stdio.h>
#include <stdlib.h>

void
ls_oom(void)
{
    fputs("Out of memory.\n", stderr);
    abort();
}
