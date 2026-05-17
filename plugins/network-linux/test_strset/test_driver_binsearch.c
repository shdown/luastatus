/*
 * Copyright (C) 2026  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "../strset_binsearch.h"
#include "../../../libls/ls_alloc_utils.h"
#include "../../../libls/ls_panic.h"

static char *global_strv[26];
static char global_buf[26 * 2];

static void init_global_stuff(void)
{
    char *cur = global_buf;
    for (size_t i = 0; i < 26; ++i) {
        cur[0] = 'a' + i;
        cur[1] = '\0';
        global_strv[i] = cur;
        cur += 2;
    }
}

static bool do_test(size_t n, const char *s)
{
    char **strv = LS_XNEW(char *, n);

    for (size_t i = 0; i < n; ++i) {
        strv[i] = global_strv[i];
    }
    bool res = strset_binsearch(strv, n, s);

    free(strv);

    return res;
}

static void perform_test_present(size_t n, size_t i)
{
    fprintf(stderr, "perform_test_present %zu %zu\n", n, i);

    LS_ASSERT(n <= 26);
    LS_ASSERT(i < n);

    char s[2] = {'a' + i, '\0'};
    bool res = do_test(n, s);
    LS_ASSERT(res);
}

static void perform_test_not_present(size_t n, size_t i)
{
    fprintf(stderr, "perform_test_not_present %zu %zu\n", n, i);

    LS_ASSERT(n <= 26);
    LS_ASSERT(i <= n);

    char s[3] = {0};
    if (i == 0) {
        // do nothing; 's' already represents an empty string
    } else {
        s[0] = 'a' + (i - 1);
        s[1] = 'a';
    }

    bool res = do_test(n, s);
    LS_ASSERT(!res);
}

int main()
{
    init_global_stuff();

    for (int n = 0; n <= 26; ++n) {
        for (int i = 0; i < n; ++i) {
            perform_test_present(n, i);
            perform_test_not_present(n, i);
        }
        perform_test_not_present(n, n);
    }

    fprintf(stderr, "PASSED\n");
}
