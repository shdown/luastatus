#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "libls/ls_strarr.h"

typedef struct {
    char *z_;
    size_t avail_;
} SAFE_STRING;

SAFE_STRING safe_string_new(void);

int safe_string_bounded_len(SAFE_STRING s, int bound);

int safe_string_getline(FILE *f, SAFE_STRING *dst);

void safe_string_free(SAFE_STRING s);

#define SAFE_STRING_FMT_ARG(S_, Bound_) \
    safe_string_bounded_len((S_), (Bound_)), (S_).z_

typedef enum {
    RESP_OK,
    RESP_ACK,
    RESP_OTHER,
} ResponseType;

bool is_good_greeting(SAFE_STRING s);

ResponseType response_type(SAFE_STRING s);

void write_quoted(FILE *f, const char *s);

// If /line/ is of form "key: value", appends /key/ and /value/ to /sa/.
void append_line_to_kv_strarr(LS_StringArray *sa, SAFE_STRING line);
