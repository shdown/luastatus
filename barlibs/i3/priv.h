#ifndef priv_h_
#define priv_h_

#include "libls/string.h"
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    // number of widgets
    size_t nwidgets;
    // content of widgets
    LSString *bufs;
    // input file descriptor
    int in_fd;
    // fdopen'ed output file descritor
    FILE *out;
    // whether no_click_events option was specified
    bool noclickev;
    // whether no_separators option was specified
    bool noseps;
    // buffer for pango_escape Lua function
    LSString luabuf;
} Priv;

#endif
