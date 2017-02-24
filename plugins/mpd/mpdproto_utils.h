#ifndef proto_utils_h_
#define proto_utils_h_

#include <string.h>
#include <stdio.h>

typedef enum {
    MPDPROTO_RESP_TYPE_OK,
    MPDPROTO_RESP_TYPE_ACK,
    MPDPROTO_RESP_TYPE_OTHER,
} MPDProtoResponseType;

static inline
MPDProtoResponseType
mpdproto_response_type(const char *buf)
{
    if (strcmp(buf, "OK\n") == 0) {
        return MPDPROTO_RESP_TYPE_OK;
    }
    if (strncmp(buf, "ACK ", 4) == 0) {
        return MPDPROTO_RESP_TYPE_ACK;
    }
    return MPDPROTO_RESP_TYPE_OTHER;
}

static inline
void
mpdproto_write_quoted(FILE *f, const char *s)
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
