This plugin monitors inotify events.

Options
===
* `watch`: table

    A table. Keys are paths to watch, values are comma-separated event names (see the “events and flag names” section).

* `greet`: boolean

   Whether or not a first call to `cb` with a `nil` argument should be made as soon as the widget starts. Defaults to false.

`cb` argument
===
If the `greet` option has been set to `true`, the first call is made with a `nil` argument.

Otherwise, it is a table with the following entries:

  * `wd`: integer

    Watch descriptor (see the “Functions” section) of the event.

  * `mask`: table

    For each event name or event flag (see the “Events and flags names” section), this table contains an entry with key equal to its name and `true` value.

  * `cookie`: number

    Unique cookie associating related events (or, if there are no associated related events, is zero).

  * `name`: string (optional)

    Present only when an event is returned for a file inside a watched directory; it identifies the filename within to the watched directory.

Functions
===
* `wds = luastatus.plugin.get_initial_wds()`

* `wd = luastatus.plugin.add_watch(path, events)`

* `is_ok = luastatus.plugin.remove_watch(wd)`

Events and flags names
===
Each `IN_*` constant defined in `<sys/inotify.h>` corresponds to a string obtained from its name by dropping the initial `IN_` and making the rest lower-case, e.g. `IN_CLOSE_WRITE` corresponds to `"close_write"`.

See http://man7.org/linux/man-pages/man7/inotify.7.html for details.
