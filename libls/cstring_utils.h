#ifndef ls_cstring_utils_h_
#define ls_cstring_utils_h_

#include <stddef.h>
#include <string.h>

#include "compdep.h"
#include "algo.h"

// If /nbuf/ is not /0/, copies /min(strlen(src), nbuf - 1)/ characters from /src/ to /buf/,
// zero-terminating the result.
// Returns /strlen(src)/.
LS_INHEADER
size_t
ls_strlcpy(char *buf, const char *src, size_t nbuf)
{
    const size_t nsrc = strlen(src);
    if (nbuf) {
        const size_t ncopy = LS_MIN(nsrc, nbuf - 1);
        // both /buf/ and /src/ are safe to dereference here, so it's safe to call memcpy without
        // additional checks (see /DOCS/c_notes/empty-ranges-and-c-stdlib.md).
        memcpy(buf, src, ncopy);
        buf[ncopy] = '\0';
    }
    return nsrc;
}

// If zero-terminated string /a/ starts with zero-terminated string /b/, returns /a + strlen(b)/.
// Otherwise, returns /NULL/.
LS_INHEADER
const char *
ls_strfollow(const char *a, const char *b)
{
    const size_t nb = strlen(b);
    return strncmp(a, b, nb) == 0 ? a + nb : NULL;
}

#endif
