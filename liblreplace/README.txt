This library provides replacements for non-thread-safe default implementations of Lua's
standard library functions.

Read '../libhackyfix/README.txt' for details.

How to use:
  1. Just after creating a new lua_State, call 'liblreplace_panic_handler_install(L)'.
  2. Call 'liblreplace_inject_all(L)' before running any user code.
