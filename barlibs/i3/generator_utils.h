#ifndef generator_utils_h_
#define generator_utils_h_

#include <stddef.h>
#include <stdbool.h>

#include "libls/string_.h"

void
append_json_escaped_s(LSString *s, const char *zts);

bool
append_json_number(LSString *s, double value);

#endif
