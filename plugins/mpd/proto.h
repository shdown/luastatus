#ifndef proto_h_
#define proto_h_

#include <string.h>
#include <stdio.h>

#include "libls/compdep.h"

typedef enum {
    RESP_OK,
    RESP_ACK,
    RESP_OTHER,
} ResponseType;

LS_INHEADER
ResponseType
response_type(const char *s)
{
    if (strcmp(s, "OK\n") == 0) {
        return RESP_OK;
    }
    if ((ls_strfollow(s, "ACK "))) {
        return RESP_ACK;
    }
    return RESP_OTHER;
}

LS_INHEADER
void
write_quoted(FILE *f, const char *s)
{
    fputc('"', f);
    for (const char *t; (t = strchr(s, '"'));) {
        fwrite(s, 1, t - s, f);
        fputs("\\\"", f);
        s = t + 1;
    }
    fputs(s, f);
    fputc('"', f);
}

#endif
