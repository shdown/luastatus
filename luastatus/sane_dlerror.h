#ifndef sane_dlsym_dlerror_h_
#define sane_dlsym_dlerror_h_

#include <dlfcn.h>
#include "libls/compdep.h"

LS_INHEADER
const char *
sane_dlerror(void)
{
    const char *err = dlerror();
    return err ? err : "(no error, but the symbol is NULL)";
}

#endif
