This barlib talks to the **wmii** (**wmii2**, **wmii3**) window manager via the 9P protocol using libixp.

It does not provide functions and does not support events.

`cb` return value
===
A string or `nil`.

Options
===
* `wmii3`

  **Required if wmii3 is used.**

* `wmii3_addline=<string>`

  E.g. `colors #000000 #ffffff #ff0000`.

* `bar=<character>`

  where `<character>` is either `l` or `r`, select which bar to use: left one or right one.

* `address=<address>`

  Address, in the Plan 9 resource format, at which to connect to the wmii 9P server.
  Default is to use `WMII_ADDRESS` environment variable.
