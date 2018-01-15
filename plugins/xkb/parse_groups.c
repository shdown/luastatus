#include "parse_groups.h"

#include <stddef.h>
#include <string.h>

#include "libls/strarr.h"

void
parse_groups(LSStringArray *groups, const char *layout)
{
    ls_strarr_clear(groups);
    int balance = 0;
    size_t prev = 0;
    const size_t nlayout = strlen(layout);
    for (size_t i = 0; i < nlayout; ++i) {
        switch (layout[i]) {
        case '(':
            ++balance;
            break;
        case ')':
            --balance;
            break;
        case ',':
            if (balance == 0) {
                ls_strarr_append(groups, layout + prev, i - prev);
                prev = i + 1;
            }
            break;
        }
    }
    ls_strarr_append(groups, layout + prev, nlayout - prev);
}
