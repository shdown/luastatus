This plugin monitors inotify events.

Options
===
* `watch`: table

    A table. Keys are paths to watch, values are comma-separated event names (see the “events and flag names” section).

* `greet`: boolean

    Whether or not a first call to `cb` with `nil` argument should be made as soon as the widget starts. Defaults to false.

* `timeout`: number

    If specified and not negative, this plugin calls `cb` with `nil` argument if no event has occured in `timeout` seconds.

`cb` argument
===
The argument is `nil` if:

    * this is the first call, and `greet` has been set to `true`;

    * no event has occured in `timeout` seconds.

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
Each file being watched is assigned a *watch descriptor*, which is a non-negative integer.

* `wds = luastatus.plugin.get_initial_wds()`

Returns a table that maps *initial* paths to their watch descriptors.

* `wd = luastatus.plugin.add_watch(path, events)`

Add a new file to watch. Returns a watch descriptor on success, or `nil` on failure.

* `is_ok = luastatus.plugin.remove_watch(wd)`

Removes a watch by its watch descriptor. Returns `true` on success, `false` on failure.

Events and flags names
===
Each `IN_*` constant defined in `<sys/inotify.h>` corresponds to a string obtained from its name by dropping the initial `IN_` and making the rest lower-case, e.g. `IN_CLOSE_WRITE` corresponds to `"close_write"`.

See http://man7.org/linux/man-pages/man7/inotify.7.html for details.
