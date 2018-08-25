This plugin monitors the volume and mute status of the default PulseAudio sink.

Options
===

None.

`cb` argument
===

A table with the following entries:

  * `cur` (integer): current volume level;

  * `norm` (integer): “normal” (corresponding to 100%) volume level;

  * `mute` (boolean): whether the sink is mute.
