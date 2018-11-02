#include "parse_int.h"

#include <limits.h>
#include <errno.h>

int
ls_strtou_b(const char *s, size_t ns, const char **endptr)
{
    int ret = 0;
    size_t i = 0;

    for (; i != ns; ++i) {
        int digit = ((int) s[i]) - '0';
        if (digit < 0 || digit > 9) {
            break;
        }
        if (ret > INT_MAX / 10) {
            ret = -ERANGE;
            break;
        }
        ret *= 10;
        if (ret > INT_MAX - digit) {
            ret = -ERANGE;
            break;
        }
        ret += digit;
    }

    *endptr = s + i;
    return ret;
}

int
ls_full_strtou_b(const char *s, size_t ns)
{
    if (!ns) {
        return -EINVAL;
    }
    const char *endptr;
    int r = ls_strtou_b(s, ns, &endptr);
    if (r < 0) {
        return r;
    }
    if (endptr != s + ns) {
        return -EINVAL;
    }
    return r;
}
