#ifndef priv_h_
#define priv_h_

#include "libls/string.h"
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    size_t nwidgets;
    LSString *bufs;
    int in_fd;
    FILE *out;
    bool noclickev;
    bool noseps;
    LSString luabuf;
} Priv;

#endif
