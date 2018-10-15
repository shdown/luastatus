This plugin subscribes to D-Bus signals.

Options
===
  * `greet`: boolean

      Whether or not a first call to `cb` with `what="hello"` should be made as soon as the widget starts. Defaults to false.

  * `timeout`: number

      If specified and not negative, this plugin calls `cb` with `what="timeout"` if no signal has been received in `timeout` seconds.

  * `signals`: array (that is, a table with numeric keys)

       Array of tables with the following entries (all are optional).

       * `sender`: string

         Sender name to match on (unique or well-known name).

       * `interface`: string

         D-Bus interface name to match on.

       * `signal`: string

         D-Bus signal name to match on.

       * `object_path`: string

         Object path to match on.

       * `arg0`: string

          Contents of first string argument to match on.

       * `flags`: array (that is, a table with numeric keys)

         Array of strings. The following are recognized:

          + `match_arg0_namespace`

            Match first arguments that contain a bus or interface name with the given namespace.

          + `match_arg0_path`

            Match first arguments that contain an object path that is either equivalent to the given path, or one of the paths is a subpath of the other.

       * `bus`: string

         Specify the bus to subscribe to the signal on: either `system` or `session`; default is `session`.

`cb` argument
===

  A table with a `what` entry.

  * If it is `"hello"`, this is the first call to `cb` (only if the `greet` option was set to `true`).

    * It it is `"timeout"`, a signal has not been received for the number of seconds specified as the `timeout` option.

  * If it is `"signal"`, a signal has been received. In this case, the table has the following additional entries:

    + `sender`: string

    Unique sender name.

    + `object_path`: string

    Object path.

    + `interface`: string

    D-Bus interface name.

    + `signal`: string

    Signal name.

    + `parameters`: *D-Bus object*

    Signal arguments.

D-Bus objects
===

D-Bus objects are marshalled as follows:

* booleans: as Lua booleans;

* byte, int16, uint16, int32, uint32, int64, uint64: as strings;

* doubles: as numbers;

* strings, object paths and signatures: as strings;

* “maybe”s: as-is, or as *special objects* with value `"nothing"`;

* handles: as *special objects* with value `"handle"`;

* arrays, tuples and “dict entries”: as Lua arrays (tables with keys 1, 2, …);

If an object cannot be marshalled, a *special object* with an error is generated instead.

Special objects
---

Special objects represent D-Bus objects that cannot be marshalled to Lua.

A special object is a function that, when called, returns either:

    * value;

    * `nil`, error.
