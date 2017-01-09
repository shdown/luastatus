#ifndef pth_check_h_
#define pth_check_h_

#include "libls/compdep.h"

#define PTH_CHECK(Expr_) pth_check__impl(Expr_, #Expr_, __FILE__, __LINE__)

void
pth_check__failed(int retval, const char *expr, const char *file, int line);

LS_INHEADER
void
pth_check__impl(int retval, const char *expr, const char *file, int line)
{
    if (retval) {
        pth_check__failed(retval, expr, file, line);
    }
}

#endif
