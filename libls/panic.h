#ifndef ls_panic_h_
#define ls_panic_h_

#include <stdio.h>
#include <stdlib.h>

#define LS_PANIC__CAT(X_) #X_

#define LS_PANIC__EVAL_CAT(X_) LS_PANIC__CAT(X_)

#define LS_PANIC(Msg_) \
    do { \
        fputs("LS_PANIC at " __FILE__ ":" LS_PANIC__EVAL_CAT(__LINE__) ": " Msg_ "\n", \
              stderr); \
        abort(); \
    } while (0)

#endif
