This derived plugin periodically polls Linux `procfs` for memory usage.

Functions
===

- `get_usage()`: returns a table with two entries, `avail` and `total`. Both have `value` (a number) and `unit` (a string) entries;
  you can assume both units are always `kB`s.

- `widget(tbl)`

    Constructs a `widget` table required by luastatus. `tbl` is a table with
    the following fields:

    **(required)**

    * `cb`: a callback function that will be called with a table same as that returned by `get_usage()`;

    **(optional)**

    * `timer_opts`: options for the underlying `timer` plugin;

    * `event`: event entry of the resulting table.
