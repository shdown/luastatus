#ifndef ls_cstring_utils_h_
#define ls_cstring_utils_h_

#include <stddef.h>
#include <string.h>
#include "compdep.h"
#include "algo.h"

// Copies up to nbuf - 1 characters from the string src to dst, zero-terminating the result if nbuf
// is not 0.
//
// Returns strlen(src).
LS_INHEADER
size_t
ls_strlcpy(char *buf, const char *src, size_t nbuf)
{
    const size_t nsrc = strlen(src);
    if (nbuf) {
        const size_t ncopy = LS_MIN(nsrc, nbuf - 1);
        memcpy(buf, src, ncopy);
        buf[ncopy] = '\0';
    }
    return nsrc;
}

// Appends string src to the end of buf. It will append at most nbuf - strlen(buf) - 1 characters.
// It will then zero-terminate, unless nbuf is 0 or the original buf string was longer than nbuf
// (in practice this should not happen as it means that either nbuf is incorrect or that buf is not
// a proper string).
//
// If the src and buf strings overlap, the behavior is undefined.
//
// Returns strnlen(buf, nbuf).
LS_INHEADER
size_t
ls_strlcat(char *buf, const char *src, size_t nbuf)
{
    const size_t nbufstr = strnlen(buf, nbuf);
    return nbufstr + ls_strlcpy(buf + nbufstr, src, nbuf - nbufstr);
}

// If zero-terminated string a starts with zero-terminated string b, returns a + strlen(b).
//
// Otherwise, returns NULL.
LS_INHEADER
const char *
ls_strfollow(const char *a, const char *b)
{
    const size_t nb = strlen(b);
    return strncmp(a, b, nb) == 0 ? a + nb : NULL;
}

#endif
