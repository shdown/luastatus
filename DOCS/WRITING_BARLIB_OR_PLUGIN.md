Thread-safety
===
Each non-thread-safe thing must be synchronized with other entities by means of the `map_get`
function (see `DOCS/design/map_get.md`).

Environment variables
===
Your plugin or barlib must not modify environment variables (this includes `unsetenv()`, `setenv()`,
`putenv()`, and modifying `environ`).

Signals
===
Your plugin or barlib can install signal handlers, but:

  1. this must be done with `sigaction()`, and `sa_mask` field of `struct sigaction` must include
     `SA_RESTART`;

  2. use the `map_get` facility to ensure there are no conflicting handlers installed; use, for
     example, the key of `"flag:signal_handled:SIGUSR1"` for signal `SIGUSR1`; the value behind the
     key should be checked to be null and then set to some non-null pointer. For POSIX real-time
     signals, use `"SIGRTMIN+%d"` nomenclature, where `%d` is a non-negative offset in decimal.

Your plugin or barlib can call `pthread_sigmask()` (and, consequently, `pselect()`).

Writing a plugin
===
Copy `include/plugin_data.h`, `include/plugin_data_v1.h`, `include/plugin_v1.h` and `include/common.h`;
include `include/plugin_v1.h` and start reading `include/plugin_data.h`.

Then, declare a global `const LuastatusIfacePlugin luastatus_iface_plugin_v1` variable.

Writing a barlib
===
Copy `include/barlib_data.h`, `include/barlib_data_v1.h`, `include/barlib_v1.h` and `include/common.h`;
include `include/barlib_v1.h` and start reading `include/barlib_data.h`.

Then, declare a global `const LuastatusIfaceBarlib luastatus_iface_barlib_v1` variable.
