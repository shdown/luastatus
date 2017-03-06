This plugin monitors inotify events.

Options
===
* `watch`: table

    A table. Keys are paths to watch, values are comma-separated event names (see the “events and flag names” section).

`cb` argument
===
A table with the following entries:

  * `path`: string

    Path of the file that the event is related to.

  * `mask`: table

    For each event name or event flag (see the “events and flags names” section), this table contains an entry with key equal to its name and `true` value.

  * `cookie`: number

    Unique cookie associating related events (or, if there are no associated related events, is zero).

  * `name`: string (optional)

    Present only when an event is returned for a file inside a watched directory; it identifies the filename within to the watched directory.

Events and flags names
===
Each `IN_*` constant defined in `<sys/inotify.h>` corresponds to a string obtained from its name by dropping the initial `IN_` and making the rest lower-case, e.g. `IN_CLOSE_WRITE` corresponds to `"close_write"`.

See http://man7.org/linux/man-pages/man7/inotify.7.html for details.
