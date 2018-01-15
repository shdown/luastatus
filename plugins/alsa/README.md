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

  Whether or not this is a capture stream, as opposed to a playback one. Defaults to false.

`cb` argument
===
A table with the following entries:
* `mute`: boolean (if the channel has a playback switch capability)
* `vol`: table with the following entries:
  * `cur`, `min`, `max`: numbers (if the channel has volume control capability)
