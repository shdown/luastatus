#ifndef generator_utils_h_
#define generator_utils_h_

#include "libls/string_.h"
#include <stddef.h>
#include <stdbool.h>

void
json_ls_string_append_escaped_s(LSString *s, const char *zts);

bool
json_ls_string_append_number(LSString *s, double value);

void
pango_ls_string_append_escaped_b(LSString *s, const char *buf, size_t nbuf);

#endif
