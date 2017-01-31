This barlib talks with `lemonbar`.

It joins all non-empty strings returned by widgets by a separator, which defaults to ` | `.

Redirections and `luastatus-lemonbar-launcher`
===
`lemonbar` requires all the data to be written to stdout and read from stdin. This makes it very easy to mess things up: Lua’s `print()` prints to stdout, processes spawned by widgets/plugins inherit our stdin and stdout, etc.
That’s why this barlib requires that stdin and stdout file descriptors are manually redirected.
A launcher, `luastatus-lemonbar-launcher`, is shipped with it; it spawns `lemonbar` (or whatever is in `LEMONBAR` environment variable) connected to a bidirectional pipe and executes `luastatus` (or whatever is in `LUASTATUS` environment variable) with `-b lemonbar` (or whatever is in `LUASTATUS_LEMONBAR_BARLIB` environment variable), all the required `-B` options, and additional arguments passed by you.
Note that environment variables are split by `IFS`, which is space, tab and newline by default. If you want pass to `lemonbar` arguments containing spaces, you should create your own wrapper and set it as `LEMONBAR` environment variable before launching `luastatus-lemonbar-launcher`.

`cb` return value
===
A string with lemonbar markup, or nil. Nil is equivalent to an empty string.

Functions
===
`escape` escapes text for lemonbar markup.

`event` argument
===
A string with a name of the command.

Options
===
* `in_fd=<fd>`

  File descriptor to read `lemonbar` input from. Usually set by the launcher.

* `out_fd=<fd>`

  File descriptor to write to. Usually set by the launcher.

* `separator=<string>`

  Set the separator.
