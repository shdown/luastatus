Thread-safety
===
Each non-thread-safe thing must be synchronized with other entities by means of
the `map_get` function (see `DOCS/design/map_get.md`).

Misc
===
Your plugin or barlib must not modify environment variables (this includes
`unsetenv()`, `setenv()`, `putenv()`, and modifying `environ`).

Your plugin or barlib can install signal handlers, but:

  1. this must be done with `sigaction()`, and `sa_mask` field of
  `struct sigaction` must include `SA_RESTART`.

  2. the handler must not be `SIG_IGN`, as it gets inherited by child
  processes;

  3. you must add a taint with the name of the signal, e.g. `"SIGUSR1"`.

Your plugin or barlib can call `pthread_sigmask()` (and, consequently,
`pselect()`).

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
