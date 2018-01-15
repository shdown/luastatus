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

void
pango_ls_string_append_escaped_b(LSString *s, const char *buf, size_t nbuf)
{
    size_t prev = 0;
    for (size_t i = 0; i < nbuf; ++i) {
        const char *esc;
        switch (buf[i]) {
        case '&':  esc = "&amp;";   break;
        case '<':  esc = "&lt;";    break;
        case '>':  esc = "&gt;";    break;
        case '\'': esc = "&apos;";  break;
        case '"':  esc = "&quot;";  break;
        default: continue;
        }
        ls_string_append_b(s, buf + prev, i - prev);
        ls_string_append_s(s, esc);
        prev = i + 1;
    }
    ls_string_append_b(s, buf + prev, nbuf - prev);
}
