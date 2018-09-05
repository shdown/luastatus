This barlib talks to the **wmii** (**wmii2**, **wmii3**) window manager via the 9P protocol using libixp.

It does not provide functions.

`cb` return value
===
A string, an array (that is, a table with integer keys) of strings, or `nil`.

Options
===
* `wmii3`

  **Required if wmii3 is used.**

* `wmii3_addline=<line>`

  Add `<line>` to all the block files. Not much use of it, unless you want to override the color settings for all of your widgets.

  E.g. `colors #000000 #ffffff #ff0000`.

* `bar=<character>`

  Select which bar to use: left (`<character>` is `l`) or right (`<character>` is `r`) one. Default is the right one.

* `address=<address>`

  Address, in the Plan 9 resource format, at which to connect to the wmii 9P server.
  Default is to use `WMII_ADDRESS` environment variable.

* `event_fd=<fd>`

  libixp is crappy and just crashes the entire process when attempts to do multithreading while claiming to be totally thread-safe.

  So, if you want your mouse clicks to be reported to widgets, use the `luastatus-wmii-wrapper` script to launch luastatus.

  Note this option (and thus using the script) is not required.

`event` argument
===

A table with the following entries:

* `button`: an integer specifying the mouse button the block was clicked with;

* `event`: a string; either `"MouseDown"` or `"Click"`.
