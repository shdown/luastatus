#ifndef priv_h_
#define priv_h_

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "libls/string_.h"

// Barlib's private data
typedef struct {
    // Number of widgets.
    size_t nwidgets;

    // Content of the widgets.
    LSString *bufs;

    // Temporary buffer for secondary buffering, to avoid unneeded redraws.
    LSString tmpbuf;

    // Input file descriptor number.
    int in_fd;

    // /fdopen/'ed output file descritor.
    FILE *out;

    // Whether or not the /no_click_events/ option was specified.
    bool noclickev;

    // Whether or not the /no_separators/ option was specified
    bool noseps;
} Priv;

#endif
