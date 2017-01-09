#ifndef ls_cstring_utils_h_
#define ls_cstring_utils_h_

#include <stddef.h>
#include <string.h>
#include "compdep.h"
#include "algo.h"

LS_INHEADER
size_t
ls_strlcpy(char *buf, const char *src, size_t nbuf)
{
    size_t nsrc = strlen(src);
    if (nbuf) {
        size_t ncopy = LS_MIN(nsrc, nbuf - 1);
        memcpy(buf, src, ncopy);
        buf[ncopy] = '\0';
    }
    return nsrc;
}

LS_INHEADER
size_t
ls_strlcat(char *buf, const char *src, size_t nbuf)
{
    size_t nbufstr = strnlen(buf, nbuf);
    return nbufstr + ls_strlcpy(buf + nbufstr, src, nbuf - nbufstr);
}

LS_INHEADER
const char*
ls_strfollow(const char *a, const char *b)
{
    size_t nb = strlen(b);
    return strncmp(a, b, nb) == 0 ? a + nb : NULL;
}

#endif
