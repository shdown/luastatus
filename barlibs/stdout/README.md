This barlib simply writes lines to a file descriptor.

It joins all non-empty strings returned by widgets by a separator, which defaults to ` | `.

It does not provide functions and does not support events.

`cb` return value
===
A string, an array (that is, a table with numeric keys) of strings (all non-empty elements of which are joined by the separator), or nil. Nil or an empty table is equivalent to an empty string.

Options
===
* `separator=<string>`

  Set the separator. Defaults to ` | `.

* `error=<string>`

  Set the content of an “error” segment. Defaults to `(Error)`.
