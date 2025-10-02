/*
 * Copyright (C) 2021-2025  luastatus developers
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

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/time.h>

static inline struct timespec now_ts(void)
{
    struct timespec ts;
    int rc;

#if (!defined(_POSIX_MONOTONIC_CLOCK)) || (_POSIX_MONOTONIC_CLOCK < 0)
    // CLOCK_MONOTONIC is not supported at compile-time.
    rc = clock_gettime(CLOCK_REALTIME, &ts);

#elif _POSIX_MONOTONIC_CLOCK > 0
    // CLOCK_MONOTONIC is supported both at compile-time and at run-time.
    rc = clock_gettime(CLOCK_MONOTONIC, &ts);

#else
    // CLOCK_MONOTONIC is supported at compile-time, but might or might not
    // be supported at run-time.
    rc = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (rc < 0) {
        rc = clock_gettime(CLOCK_REALTIME, &ts);
    }
#endif

    if (rc < 0) {
        fprintf(stderr, "stopwatch: clock_gettime() failed.\n");
        abort();
    }
    return ts;
}

static inline uint64_t now_ms(void)
{
    struct timespec ts = now_ts();
    return ((uint64_t) ts.tv_sec) * 1000 + (ts.tv_nsec / 1000000);
}

static uint64_t parse_uint_cstr_or_die(const char *s)
{
    uint64_t r;
    if (sscanf(s, "%" SCNu64, &r) != 1) {
        fprintf(stderr, "stopwatch: cannot parse as unsigned integer: '%s'.\n", s);
        abort();
    }
    return r;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "USAGE: stopwatch <slackness in milliseconds>\n");
        return 2;
    }
    uint64_t slackness = parse_uint_cstr_or_die(argv[1]);

    uint64_t start = 0;

    char *buf = NULL;
    size_t nbuf = 0;
    while (getline(&buf, &nbuf, stdin) > 0) {
        switch (buf[0]) {
        case 's':
            start = now_ms();
            break;
        case 'q':
            printf("%" PRIu64 "\n", now_ms() - start);
            fflush(stdout);
            break;
        case 'r':
            {
                uint64_t cur = now_ms();
                printf("%" PRIu64 "\n", cur - start);
                fflush(stdout);
                start = cur;
            }
            break;
        case 'c':
            {
                if (buf[1] != ' ') {
                    fprintf(stderr, "stopwatch: expected space after 'c', found symbol '%c'.\n", buf[1]);
                    abort();
                }
                uint64_t cur = now_ms();
                uint64_t delta = cur - start;
                uint64_t expected = parse_uint_cstr_or_die(buf + 2);
                if (delta > expected + slackness) {
                    printf("0 delta-is-too-big %" PRIu64 "\n", delta);
                } else if (expected > slackness && delta < (expected - slackness)) {
                    printf("0 delta-is-too-small %" PRIu64 "\n", delta);
                } else {
                    printf("1\n");
                }
                fflush(stdout);
                start = cur;
            }
            break;
        default:
            fprintf(stderr, "stopwatch: got line with unexpected first symbol '%c'.\n", buf[0]);
            abort();
        }
    }
    if (!feof(stdin)) {
        perror("stopwatch: getline");
        abort();
    }
    free(buf);
    return 0;
}
