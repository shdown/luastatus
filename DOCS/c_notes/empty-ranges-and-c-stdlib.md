http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf

Subclause 7.1.4 Use of library functions, point 1:

> Each of the following statements applies unless explicitly stated otherwise in the
> detailed descriptions that follow: If an argument to a function has an invalid value (such
> as a value outside the domain of the function, or a pointer outside the address space of
> the program, or a null pointer, or a pointer to non-modifiable storage when the
> corresponding parameter is not const-qualified) or a type (after promotion) not expected
> by a function with variable number of arguments, the behavior is undefined. If a function
> argument is described as being an array, the pointer actually passed to the function shall
> have a value such that all address computations and accesses to objects (that would be
> valid if the pointer did point to the first element of such an array) are in fact valid.

It’s not OK to call any function defined in `<string.h>` with a pointer that can’t be dereferenced:

Subclause 7.21.1 String handling conventions, point 2:

> Where an argument declared as `size_t n` specifies the length of the array for a function, `n`
> can have the value zero on a call to that function. Unless explicitly stated otherwise in
> the description of a particular function in this subclause, pointer arguments on such a
> call shall still have valid values, as described in 7.1.4. On such a call, a function
> that locates a character finds no occurrence, a function that compares two character
> sequences returns zero, and a function that copies characters copies zero characters.

It’s not OK to call `qsort`/`bsearch` with a pointer that can’t be dereferenced:

Subclause 7.20.5 Searching and sorting utilites, point 1:

> These utilities make use of a comparison function to search or sort arrays of unspecified
> type. Where an argument declared as `size_t nmemb` specifies the length of the array for a
> function, `nmemb` can have the value zero on a call to that function; the comparison function
> is not called, a search finds no matching element, and sorting performs no rearrangement.
> Pointer arguments on such a call shall still have valid values, as described in 7.1.4.

However, it’s OK to call `snprintf`/`vsnprintf` with a pointer that can’t be dereferenced, as long as `n` is zero:

Subclause 7.19.6.5 The `snprintf` function, point 2:

> The `snprintf` function is equivalent to `fprintf`, except that the output is written into an
> array (specified by argument `s`) rather than to a stream. If `n` is zero, nothing is written, and
> `s` may be a null pointer. Otherwise, output characters beyond the `n-1`st are discarded rather
> than being written to the array, and a null character is written at the end of the characters
> actually written into the array. If copying takes place between objects that overlap, the behavior
> is undefined.
