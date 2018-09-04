This plugin monitors the volume and mute status of the default PulseAudio sink.

Options
===

* `make_self_pipe`: boolean

If `true`, the `wake_up()` (see below) function will be available.

`cb` argument
===

* If the `make_self_pipe` option is set to `true`, and the callback is invoked because of the call to `wake_up()`, then the argument is `nil`;

* otherwise, the argument is a table with the following entries:

  - `cur` (integer): current volume level;

  - `norm` (integer): “normal” (corresponding to 100%) volume level;

  - `mute` (boolean): whether the sink is mute.

Functions
===

* `wake_up()`: if the `make_self_pipe` option is set to `true`, forces a call to `cb` (with `nil` argument).
