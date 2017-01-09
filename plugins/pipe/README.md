This plugin spaws a process and monitors its stdout. By default, it waits for the process to print a line, and calls `cb` with that line, but a custom delimiter can be specified.

Options
===
* `args`: array of strings

  Executable and arguments to spawn. Must be specified and must be non-empty.

* `delimiter`: string of length 1

  Delimiter. Defaults to `'\n'`. Note that Lua strings are not null-terminated strings, so `'\0'` is fine.

`cb` argument
===
A framgent of output between two delimiters or before a first delimiter. The delimiter itself is not included.
