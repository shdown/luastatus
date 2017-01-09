Taints
===
A taint is a thread-unsafe thing that only one plugin or barlib can do. Examples
are Xlib and installing signal handlers.

Plugins and barlibs "advertise" what taints they have via the `taints`
NULL-terminating array of strings. On startup, luastatus ensures that no two
plugins, or a plugin and the barlib, have the same taint.

Naming taints
---
If it is a shared library, the name is `lib` + its name, e.g. for Xlib, its
`"libx11"`.

If it is a signal handler, the name is the name of the signal, e.g. `"SIGUSR1"`.

If neither, please open a GitHub issue.

Threads, signals, environment variables
===
Your plugin or barlib must be thread-safe, or, for each non-thread-safe
thing, a taint must be added.

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

Writing a barlib
===
You should copy `include/barlib.h`, `include/loglevel.h`, and
`include/barlib_data.h` to your project and include `include/barlib.h`.

Then, you must declare `LuastatusBarlibIface luastatus_barlib_iface` as a
global variable.

Read `include/barlib_data.h` for more information.

Writing a plugin
===
You should copy `include/plugin.h` and `include/plugin_data.h` to your
project and include `include/plugin.h`.

Then, you must declare `LuastatusPluginIface luastatus_plugin_iface` as a
global variable.

Read `include/plugin_data.h` for more information.
