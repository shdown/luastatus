#ifndef ls_cstring_utils_h_
#define ls_cstring_utils_h_

#include <stddef.h>
#include <string.h>

#include "compdep.h"

// If zero-terminated string /str/ starts with zero-terminated string /prefix/, returns
// /str + strlen(prefix)/; otherwise, returns /NULL/.
LS_INHEADER
const char *
ls_strfollow(const char *str, const char *prefix)
{
    const size_t nprefix = strlen(prefix);
    return strncmp(str, prefix, nprefix) == 0 ? str + nprefix : NULL;
}

// Behaves like the GNU-specific /strerror_r/: either fills /buf/ and returns it, or returns a
// pointer to a static string.
const char *
ls_strerror_r(int errnum, char *buf, size_t nbuf);

// Yes, this actually works.
#define ls_strerror_onstack(Errnum_) ls_strerror_r(Errnum_, (char [256]) {'\0'}, 256)

#endif
