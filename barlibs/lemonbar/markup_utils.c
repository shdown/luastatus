#include "markup_utils.h"

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "libls/string_.h"
#include "libls/parse_int.h"

void
lemonbar_ls_string_append_escaped_b(LSString *buf, const char *s, size_t ns)
{
    // just replace all "%"s with "%%"

    // we have to check ns before calling memchr, see ../../DOCS/empty-ranges-and-c-stdlib.md
    for (const char *t; ns && (t = memchr(s, '%', ns));) {
        const size_t nseg = t - s + 1;
        ls_string_append_b(buf, s, nseg);
        ls_string_append_c(buf, '%');
        ns -= nseg;
        s += nseg;
    }
    ls_string_append_b(buf, s, ns);
}

void
lemonbar_ls_string_append_sanitized_b(LSString *buf, size_t widget_idx, const char *s, size_t ns)
{
    size_t prev = 0;
    bool a_tag = false;
    for (size_t i = 0; i < ns; ++i) {
        switch (s[i]) {
        case '\n':
            ls_string_append_b(buf, s + prev, i - prev);
            prev = i + 1;
            break;

        case '%':
            if (i + 1 < ns) {
                if (s[i + 1] == '{' && i + 2 < ns && s[i + 2] == 'A') {
                    a_tag = true;
                } else if (s[i + 1] == '%') {
                    ++i;
                }
            }
            break;

        case ':':
            if (a_tag) {
                ls_string_append_b(buf, s + prev, i + 1 - prev);
                ls_string_append_f(buf, "%zu_", widget_idx);
                prev = i + 1;
                a_tag = false;
            }
            break;

        case '}':
            a_tag = false;
            break;
        }
    }
    ls_string_append_b(buf, s + prev, ns - prev);
}

const char *
lemonbar_parse_command(const char *line, size_t nline, size_t *ncommand, size_t *widget_idx)
{
    const char *endptr;
    const int idx = ls_parse_uint_b(line, nline, &endptr);
    if (idx < 0 ||
        endptr == line ||
        endptr == line + nline ||
        *endptr != '_')
    {
        return NULL;
    }
    const char *command = endptr + 1;
    *ncommand = nline - (command - line);
    *widget_idx = idx;
    return command;
}
