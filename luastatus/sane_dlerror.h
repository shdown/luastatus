#ifndef sane_dlsym_dlerror_h_
#define sane_dlsym_dlerror_h_

#include <dlfcn.h>
#include "libls/compdep.h"

// http://pubs.opengroup.org/onlinepubs/9699919799/functions/dlerror.html
//
// > It is implementation-defined whether or not the dlerror() function is thread-safe.
//
// So, it is implementation-defined whether or not sane_dlerror is thread-safe.
LS_INHEADER
const char *
sane_dlerror(void)
{
    const char *err = dlerror();
    return err ? err : "(no error, but the symbol is NULL)";
}

#endif
