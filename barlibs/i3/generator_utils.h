#ifndef generator_utils_h_
#define generator_utils_h_

#include <stddef.h>
#include <stdbool.h>

#include "libls/string_.h"

void
json_ls_string_append_escaped_s(LSString *s, const char *zts);

bool
json_ls_string_append_number(LSString *s, double value);

#endif
