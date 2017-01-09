This plugin is a simple timer, plus a wake-up FIFO can be specified.

Options
===
* `period`: number

  A number of seconds to sleep before calling `cb` again. May be fractional. Defaults to 1.

* `fifo`: string

  Path to an existent FIFO. The plugin does not create FIFO itself. To force a wake-up, `touch(1)` the FIFO, that is, open it for writing and then close.

`cb` argument
===
A string decribing why the function is called:

* If it’s `"hello"`, the function is called for the first time.

* If it’s `"timeout"`, the function hasn’t been called for `period` seconds.

* If it’s `"fifo"`, the FIFO has been touched.
