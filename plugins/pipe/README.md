This derived plugin monitors the output of a process and calls the callback
function whenever it produces a line.

Functions
===
- `shell_escape(x)`, `x` is either a string or an array of strings: escapes an
  argument or an array of arguments, joining them by space, for shell.

- `widget(tbl)`

    Constructs a `widget` table required by luastatus. `tbl` is a table with
    the following fields:

    **(required)**

    * `command`: `/bin/sh` command to spawn;

    * `cb`: a callback function that will be called with a line produces by
    the spawned process each time it does so;

    **(optional)**

    * `on_eof`: a function to be called when the spawned process closes its
    stdout. Default handler simply hangs to prevent busy-looping forever;

    * `event`: event entry of the resulting table.
