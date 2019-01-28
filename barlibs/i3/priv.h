#ifndef priv_h_
#define priv_h_

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "libls/string_.h"

typedef struct {
    size_t nwidgets;

    LSString *bufs;

    // Temporary buffer for secondary buffering, to avoid unneeded redraws.
    LSString tmpbuf;

    // Input file descriptor.
    int in_fd;

    // /fdopen/'ed output file descritor.
    FILE *out;

    bool noclickev;

    bool noseps;
} Priv;

#endif
