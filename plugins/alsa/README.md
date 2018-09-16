This plugin monitors volume and mute state of an ALSA channel.

Options
===
* `card`: string

  Card name, defaults to `default`.

* `channel`: string

  Channel name, defaults to `Master`.

* `in_db`: boolean

  Whether or not to report normalized volume (in dBs).

* `capture`: boolean

  Whether or not this is a capture stream, as opposed to a playback one.
  Defaults to false.

* `make_self_pipe`: boolean

  If true, the `wake_up()` (see below) function will be available.
  Defaults to false.

`cb` argument
===
A table with the following entries:
* `mute`: boolean (if the channel has a playback switch capability)
* `vol`: table with the following entries:
  * `cur`, `min`, `max`: numbers (if the channel has volume control capability)

Functions
===

* `wake_up()`: if the `make_self_pipe` option is set to `true`, forces a call to
`cb`.
