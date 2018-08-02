This barlib simply writes lines to a file descriptor.

It can be used for status bars such as **dzen**/**dzen2**, **xmobar**, **yabar**, **dvtm**, and others.

It joins all non-empty strings returned by widgets by a separator, which defaults to ` | `.

It does not provide functions and does not support events.

Redirections and `luastatus-stdout-wrapper`
===
It is very easy to mess things up: Lua’s `print()` prints to stdout, processes spawned by widgets/plugins inherit our stdin and stdout, etc.

That’s why this barlib requires that stdout file descriptor is manually redirected. A shell wrapper, `luastatus-stdout-wrapper`, is shipped with it; it does all the redirections needed and executes `luastatus` (or whatever is in `LUASTATUS` environment variable) with `-b stdout` and additional arguments passed by you.

`cb` return value
===
A string, an array (that is, a table with numeric keys) of strings (all non-empty elements of which are joined by the separator), or nil. Nil or an empty table is equivalent to an empty string.

Options
===
* `out_fd=<fd>`

  File descriptor to write to. Usually set by the wrapper.

* `separator=<string>`

  Set the separator. Defaults to ` | `.

* `error=<string>`

  Set the content of an “error” segment. Defaults to `(Error)`.
