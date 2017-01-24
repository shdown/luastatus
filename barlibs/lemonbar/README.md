This barlib talks with `lemonbar`.

It joins all non-empty strings returned by widgets by a separator, which defaults to ` | `.

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

  File descriptor to read `lemonbar` input from. Usually set by the wrapper.

* `out_fd=<fd>`

  File descriptor to write to. Usually set by the wrapper.

* `separator=<string>`

  Set the separator.
