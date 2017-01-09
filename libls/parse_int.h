#ifndef ls_parse_int_h_
#define ls_parse_int_h_

#include <stddef.h>

int
ls_parse_uint(const char *s, size_t ns, char const **endptr);

int
ls_full_parse_uint(const char *s, size_t ns);

int
ls_full_parse_uint_cstr(const char *s);

#endif
