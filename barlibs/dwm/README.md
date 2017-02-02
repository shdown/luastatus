This barlib sets the name of the root window.

It joins all non-empty strings returned by widgets by a separator, which defaults to ` | `.

It does not provide functions and does not support events.

`cb` return value
===
A string, an array (that is, a table with numeric keys) of strings (all non-empty elements of which are joined by the separator), or nil. Nil or an empty table is equivalent to an empty string.

Options
===
* `display=<name>`

  Set the name of a display to connect to. Default is to use `DISPLAY` environment variable.

* `separator=<string>`

  Set the separator.
