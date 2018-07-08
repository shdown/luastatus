This plugin monitors inotify events.

Options
===
* `watch`: table

    A table. Keys are paths to watch, values are comma-separated event names (see the “events and flag names” section).

* `greet`: boolean

    Whether or not to call `cb` with `what="hello"` as soon as the widget starts. Defaults to false.

* `timeout`: number

    If specified and not negative, this plugin calls `cb` with `what="timeout"` if no event has occured in `timeout` seconds.

`cb` argument
===
A table with `what` entry.

- If it is `"hello"`, the function is being called for the first time (and the `greet` option was set to `true`);

- if it is `"timeout"`, the function has not been called for the number of seconds specified as the `timeout` option;

- if it is `"event"`, an inotify event has been read; in this case, the table has the following additional entries:

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
