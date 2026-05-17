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
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "../strset_mergesort.h"
#include "../../../libls/ls_alloc_utils.h"
#include "../../../libls/ls_algo.h"
#include "../../../libls/ls_panic.h"
#include "minstd.h"

static MINSTD_Prng prng;

typedef struct {
    uint16_t *data;
    size_t size;
} IntSlice;

static IntSlice alloc_int_slice(size_t n)
{
    uint16_t *data = LS_XNEW(uint16_t, n);
    return (IntSlice) {.data = data, .size = n};
}

static void free_int_slice(IntSlice x)
{
    free(x.data);
}

static void print_out_strv(const char *what, char **data, size_t ndata)
{
    fprintf(stderr, "%s =>", what);
    for (size_t i = 0; i < ndata; ++i) {
        fprintf(stderr, " %s", data[i]);
    }
    fprintf(stderr, "\n");
}

static inline void write_base26_str(char *dst, uint16_t a, size_t ndigits)
{
    for (size_t i = ndigits - 1; i != (size_t) -1; --i) {
        dst[i] = 'a' + (a % 26);
        a /= 26;
    }
    dst[ndigits] = '\0';
}

static int my_compare_callback(const void *vp, const void *vq)
{
    char *p = *(char **) vp;
    char *q = *(char **) vq;
    return strcmp(p, q);
}

static void do_test(IntSlice x)
{
    enum { D = 4 };
    char *buf = ls_xmalloc(x.size, D + 1);
    char **ptrs_a = LS_XNEW(char *, x.size);
    char **ptrs_b = LS_XNEW(char *, x.size);

    char *cur = buf;
    for (size_t i = 0; i < x.size; ++i) {
        write_base26_str(cur, x.data[i], D);
        ptrs_a[i] = cur;
        ptrs_b[i] = cur;
        cur += D + 1;
    }

    if (x.size) {
        qsort(ptrs_a, x.size, sizeof(char *), my_compare_callback);
    }
    strset_mergesort(ptrs_b, x.size);

    for (size_t i = 0; i < x.size; ++i) {
        if (strcmp(ptrs_a[i], ptrs_b[i]) != 0) {
            fprintf(stderr, "Mismatch at position %zu\n", i);
            print_out_strv("a", ptrs_a, x.size);
            print_out_strv("b", ptrs_b, x.size);
            exit(1);
        }
    }

    free(ptrs_a);
    free(ptrs_b);
    free(buf);
}

static bool next_base_n_number(IntSlice x, uint16_t n)
{
    for (size_t i = 0; i < x.size; ++i) {
        if (x.data[i] < n - 1) {
            ++x.data[i];
            return true;
        }
        x.data[i] = 0;
    }
    return false;
}

static void test_all_combinations(uint16_t n)
{
    fprintf(stderr, "test_all_combinations %d\n", (int) n);

    IntSlice x = alloc_int_slice(n);

    for (int i = 0; i < n; ++i) {
        x.data[i] = 0;
    }

    do {
        do_test(x);
    } while (next_base_n_number(x, n));

    free_int_slice(x);
}

static void test_random(size_t n)
{
    fprintf(stderr, "test_random %zu\n", n);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        x.data[i] = minstd_prng_next_u64(&prng);
    }

    do_test(x);

    free_int_slice(x);
}

static void test_asc(size_t n)
{
    fprintf(stderr, "test_asc %zu\n", n);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        x.data[i] = i;
    }

    do_test(x);

    free_int_slice(x);
}

static void test_desc(size_t n)
{
    fprintf(stderr, "test_desc %zu\n", n);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        x.data[i] = n - i;
    }

    do_test(x);

    free_int_slice(x);
}

static void permute(IntSlice x, size_t nswaps)
{
    LS_ASSERT(x.size > 0);

    if (!nswaps) {
        nswaps = 1;
    }

    for (size_t i = 0; i < nswaps; ++i) {
        size_t j1 = minstd_prng_next_limit_u32(&prng, x.size);
        size_t j2 = minstd_prng_next_limit_u32(&prng, x.size);
        LS_SWAP(uint16_t, x.data[j1], x.data[j2]);
    }
}

static void test_almost_asc(size_t n)
{
    fprintf(stderr, "test_almost_asc %zu\n", n);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        x.data[i] = i;
    }

    permute(x, n / 20);

    do_test(x);

    free_int_slice(x);
}

static void test_almost_desc(size_t n)
{
    fprintf(stderr, "test_almost_desc %zu\n", n);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        x.data[i] = n - i;
    }

    permute(x, n / 20);

    do_test(x);

    free_int_slice(x);
}

static void test_asc_shifted(size_t n, int shift)
{
    fprintf(stderr, "test_asc_shifted %zu %d\n", n, shift);

    LS_ASSERT(n > 0);
    LS_ASSERT(shift == 1 || shift == -1);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        int j = i + shift;
        if (j < 0) {
            j += n;
        }
        x.data[i] = j % n;
    }

    do_test(x);

    free_int_slice(x);
}

static void test_desc_shifted(size_t n, int shift)
{
    fprintf(stderr, "test_desc_shifted %zu %d\n", n, shift);

    LS_ASSERT(n > 0);
    LS_ASSERT(shift == 1 || shift == -1);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        int j = (n - i - 1) + shift;
        if (j < 0) {
            j += n;
        }
        x.data[i] = j % n;
    }

    do_test(x);

    free_int_slice(x);
}

static void test_all_equal(size_t n)
{
    fprintf(stderr, "test_all_equal %zu\n", n);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        x.data[i] = 0;
    }

    do_test(x);

    free_int_slice(x);
}

static void test_all_but_one_equal(size_t n, size_t special_pos)
{
    fprintf(stderr, "test_all_but_one_equal %zu %zu\n", n, special_pos);

    LS_ASSERT(special_pos < n);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        x.data[i] = 0;
    }
    x.data[special_pos] = 1;

    do_test(x);

    free_int_slice(x);
}

static void test_mod2(size_t n)
{
    fprintf(stderr, "test_mod2 %zu\n", n);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        x.data[i] = i % 2;
    }

    do_test(x);

    free_int_slice(x);
}

static void test_random_mod(size_t n, uint32_t d)
{
    fprintf(stderr, "test_random_mod %zu %" PRIu32 "\n", n, d);

    LS_ASSERT(d > 0);

    IntSlice x = alloc_int_slice(n);

    for (size_t i = 0; i < n; ++i) {
        x.data[i] = minstd_prng_next_limit_u32(&prng, d);
    }

    do_test(x);

    free_int_slice(x);
}

static void test_long_and_short_run(size_t n, size_t nshort)
{
    fprintf(stderr, "test_long_and_short_run %zu %zu\n", n, nshort);

    LS_ASSERT(nshort < n);

    IntSlice x = alloc_int_slice(n);

    size_t nlong = n - nshort;

    for (size_t i = 0; i < n; ++i) {
        x.data[i] = i < nlong ? i : (i - nlong);
    }

    do_test(x);

    free_int_slice(x);
}

static void test_fixed_impl(const uint16_t *data, size_t ndata)
{
    fprintf(stderr, "test_fixed_impl {...} (ndata=%zu)\n", ndata);

    LS_ASSERT(ndata > 7);

    IntSlice x = alloc_int_slice(ndata);

    for (size_t i = 0; i < ndata; ++i) {
        x.data[i] = data[i];
    }
    do_test(x);

    // reverse and test again
    for (size_t i = 0; i < ndata; ++i) {
        x.data[i] = data[ndata - i - 1];
    }
    do_test(x);

    free_int_slice(x);
}

#define TEST_FIXED(...) \
    do { \
        const uint16_t arr_[] = {__VA_ARGS__}; \
        size_t n_ = LS_ARRAY_SIZE(arr_); \
        test_fixed_impl(arr_, n_); \
    } while (0)

int main()
{
    prng = minstd_prng_new(0x458e7c55u);

    for (int i = 0; i <= 7; ++i) {
        test_all_combinations(i);
    }

    for (int i = 8; i <= 256; ++i) {
        for (int j = 0; j < 25; ++j) {
            test_random(i);

            test_almost_asc(i);
            test_almost_desc(i);

            test_random_mod(i, 2);
            test_random_mod(i, 3);
        }

        test_asc(i);
        test_desc(i);

        test_asc_shifted(i, 1);
        test_asc_shifted(i, -1);

        test_desc_shifted(i, 1);
        test_desc_shifted(i, -1);

        test_all_equal(i);

        test_all_but_one_equal(i, 0);
        test_all_but_one_equal(i, i / 2);
        test_all_but_one_equal(i, i - 1);

        test_mod2(i);
    }

    test_long_and_short_run(110, 10);

    // Increase then decrease
    TEST_FIXED(1,2,3,4,5,4,3,2,1);

    // Decrease then increase
    TEST_FIXED(5,4,3,2,1,2,3,4,5);

    // Sawtooth / oscillating
    TEST_FIXED(1,3,2,4,3,5,4,6);
    TEST_FIXED(1,9,2,8,3,7,4,6);

    // Plateau
    TEST_FIXED(1,1,2,2,1,1,2,2);

    // Two sorted runs concatenated
    TEST_FIXED(1,3,5,7, 2,4,6,8);

    // Many short sorted runs
    TEST_FIXED(1,2, 5,4, 6,7, 3,2);

    fprintf(stderr, "PASSED\n");
}
