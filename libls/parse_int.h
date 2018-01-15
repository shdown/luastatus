#ifndef ls_parse_int_h_
#define ls_parse_int_h_

#include <stddef.h>

// Parses (locale-independently) a decimal unsigned integer, inspecting no more than first /ns/
// characters of /s/. Once this limit is reached, or a non-digit character is found, this function
// stops, writes the current position to /*endptr/, and returns what has been parsed insofar (if
// nothing, /0/ is returned).
//
// If an overflow happens, /-ERANGE/ is returned.
int
ls_parse_uint_b(const char *s, size_t ns, char const **endptr);

// Parses (locale-independently) a decimal unsigned integer using first /ns/ characters of /s/.
//
// If a non-digit character is found among them, or if /ns/ is /0/, /-EINVAL/ is returned.
// If an overflow happens, /-ERANGE/ is returned.
int
ls_full_parse_uint_b(const char *s, size_t ns);

// Parses (locale-independently) a decimal unsigned integer using zero-terminated string /s/.
//
// If a non-digit character is found in /s/, or if /s/ is empty, /-EINVAL/ is returned.
// If an overflow happens, /-ERANGE/ is returned.
int
ls_full_parse_uint_s(const char *s);

#endif
