This plugin monitors udev events.

Options
===

* `subsystem`: string

    Subsystem to monitor events from. Optional.

* `devtype`: string

    Only report events with this device type. Optional.

* `tag`: string

    Only report events with this tag. Optional.

* `kernel_events`: boolean

    Monitor kernel uevents, not udev ones. Defaults to false.

* `greet`: boolean

    Whether or not to call `cb` with `what="hello"` as soon as the widget starts. Defaults to false.

* `timeout`: number

    If specified and not negative, this plugin calls `cb` with `what="timeout"` if no event has occured in `timeout` seconds.

`cb` argument
===
A table with `what` entry.

- If it is `"hello"`, the function is being called for the first time (and the `greet` option was set to `true`);

- if it is `"timeout"`, the function has not been called for the number of seconds specified as the `timeout` option;

- if it is `"event"`, a udev event has occured; in this case, the table has the following additional entries (all are optional strings):

  * `syspath`;

  * `sysname`;

  * `sysnum`;

  * `devpath`;

  * `devnode`;

  * `devtype`;

  * `subsystem`;

  * `driver`;

  * `action`.
