#include "generator_utils.h"

#include <math.h>
#include <stdbool.h>

void
json_ls_string_append_escaped_s(LSString *s, const char *zts)
{
    ls_string_append_c(s, '"');
    size_t prev = 0;
    size_t i;
    for (i = 0; ; ++i) {
        unsigned char c = zts[i];
        if (c == '\0') {
            break;
        }
        if (c < 32 || c == '\\' || c == '"' || c == '/') {
            ls_string_append_b(s, zts + prev, i - prev);
            ls_string_append_f(s, "\\u%04X", c);
            prev = i + 1;
        }
    }
    ls_string_append_b(s, zts + prev, i - prev);
    ls_string_append_c(s, '"');
}

bool
json_ls_string_append_number(LSString *s, double value)
{
    if (!isfinite(value)) {
        return false;
    }
    return ls_string_append_f(s, "%.20g", value);
}
