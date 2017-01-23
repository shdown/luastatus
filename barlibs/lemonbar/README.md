This barlib talks with `lemonbar`.

It joins all non-empty strings returned by widgets by a separator, which defaults to ` | `.

It does not support events yet.

Redirections and `luastatus-lemonbar-wrapper`
===
`lemonbar` requires all the data to be written to stdout and read from stdin. This makes it very easy to mess things up: Lua’s `print()` prints to stdout, processes spawned by widgets/plugins/barlib inherit our stdin and stdout, etc.
That’s why this barlib requires that stdin and stdout file descriptors are manually redirected. A shell wrapper, `luastatus-lemonbar-wrapper`, is shipped with it; it does all the redirections and executes `luastatus` with `-b lemonbar`, all the required `-B` options, and additional arguments passed by you.

`cb` return value
===
A string with lemonbar markup, or nil. Nil is equivalent to an empty string.

Functions
===
`escape` escapes text for lemonbar markup.

Options
===
* `in_fd=<fd>`

  File descriptor to read `lemonbar` input from. Usually set by the wrapper.

* `out_fd=<fd>`

  File descriptor to write to. Usually set by the wrapper.

* `separator=<string>`

  Set the separator.
