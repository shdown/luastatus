#include "safe_haven.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "libls/ls_alloc_utils.h"
#include "libls/ls_strarr.h"
#include "libls/ls_cstring_utils.h"

SAFE_STRING safe_string_new(void)
{
    enum { PREALLOC = 512 }; // must be positive

    char *z = LS_XNEW(char, PREALLOC);
    z[0] = '\0';

    return (SAFE_STRING) {
        .z_ = z,
        .avail_ = PREALLOC,
    };
}

int safe_string_bounded_len(SAFE_STRING s, int bound)
{
    size_t len = strlen(s.z_);
    if (len > (unsigned) bound) {
        len = bound;
    }
    return len;
}

static inline void rstrip_nl(char *z)
{
    size_t nz = strlen(z);
    if (nz && z[nz - 1] == '\n') {
        z[nz - 1] = '\0';
    }
}

int safe_string_getline(FILE *f, SAFE_STRING *dst)
{
    if (getline(&dst->z_, &dst->avail_, f) < 0) {
        return -1;
    }
    rstrip_nl(dst->z_);
    return 0;
}

void safe_string_free(SAFE_STRING s)
{
    free(s.z_);
}

bool is_good_greeting(SAFE_STRING s)
{
    return ls_strfollow(s.z_, "OK MPD ") != NULL;
}

ResponseType response_type(SAFE_STRING s)
{
    if (strcmp(s.z_, "OK") == 0) {
        return RESP_OK;
    }
    if ((ls_strfollow(s.z_, "ACK "))) {
        return RESP_ACK;
    }
    return RESP_OTHER;
}

void write_quoted(FILE *f, const char *s)
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

void append_line_to_kv_strarr(LS_StringArray *sa, SAFE_STRING line)
{
    char *z = line.z_;

    const char *colon_pos = strchr(z, ':');
    if (!colon_pos || colon_pos[1] != ' ') {
        return;
    }
    const char *value_pos = colon_pos + 2;
    ls_strarr_append(sa, z, colon_pos - z);
    ls_strarr_append(sa, value_pos, strlen(value_pos));
}
