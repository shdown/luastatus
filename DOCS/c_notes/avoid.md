Lua API
===

C functions and the `/*__OK__*/` thing
---
See `lua_cfuncs.md` file in the same directory.

Loose type checks
---
Don't use `lua_isstring()` and `lua_isnumber()`: `lua_isstring()` returns 1 on
numbers, `lua_isnumber()` returns 1 on strings convertible to numbers.
Use `lua_type()` instead.

C/POSIX stuff
===

Should be wrapped (avoid, except in wrappers):
---

`malloc()`, `calloc()`, `realloc()`, `strdup()`, etc: overflow checks are
  needed, and we need to abort on out-of-memory condition. Also
  `realloc(ptr, 0)` is undefined behavior in new C standards, and valgrind has
  started to warn on realloc-to-zero, so we need to work around it.

`strerror_r()`: the feature test macros mess.

`close()`, unless you are sure that the argument is non-negative (valgrind
  warns if `close()` is done on a negative fd; real OSes just fail with error
  `EBADFD`).

`poll()`: EINTR safety, hard to get right if polling with a timeout.

`pipe()`, `socket()`, `accept()`: CLOEXEC problems.

Avoid because not thread-safe:
---

`strerror()`, `rand()`, `srand()`, `setlocale()`, `localeconv()`, `putenv()`,
`setenv()`, `strtok()`, `exit()`, `quick_exit()`, `atexit()`, `at_quck_exit()`,
`system()`, `sleep()`.

Avoid for other reasons:
---

`rand_r()`: removed in POSIX-2024 in some reason.

`signal()`: has no consistent cross-platform semantics, except for
  `signal(signum, SIG_IGN)`. Use `sigaction()` instead.

Avoid because why would you even think of using those?
---

`gets()`.

`strcpy()`, `stpcpy()`, `strncpy()`.

`strcat()`, `strncat()`.

`sprintf()`, `vsprintf()`.

`scanf()`, `sscanf()`, `fscanf()`, `vscanf()`, `vsscanf()`, `vfscanf()`.

`atoi()`, `atol()`, `atoll()`, `atof()`.
