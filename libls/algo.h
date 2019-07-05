#ifndef ls_algo_h_
#define ls_algo_h_

#include <stdbool.h>

#include "compdep.h"

#define LS_ARRAY_SIZE(X_) (sizeof(X_) / sizeof((X_)[0]))

// Checks if /x/:
//   * is not NaN;
//   * belongs to /[lbound, ubound]/.
LS_INHEADER
bool
ls_is_between_d(double x, double lbound, double ubound)
{
    return x >= lbound && x <= ubound;
}

#endif
