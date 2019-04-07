#ifndef ls_panic_h_
#define ls_panic_h_

#include <stdio.h>
#include <stdlib.h>

// Logs /Msg_/ and aborts.
#define LS_PANIC(Msg_) \
    do { \
        fprintf(stderr, "LS_PANIC() at %s:%d: %s\n", __FILE__, __LINE__, Msg_); \
        abort(); \
    } while (0)

// Asserts that a /pthread_*/ call was successful.
#define LS_PTH_CHECK(Expr_) ls_pth_check_impl(Expr_, #Expr_, __FILE__, __LINE__)

// Implementation part for /LS_PTH_CHECK()/. Normally should not be called manually.
void
ls_pth_check_impl(int ret, const char *expr, const char *file, int line);

#endif
