This barlib talks to the **wmii** window manager using libixp.

It does not provide functions and does not support events.

`cb` return value
===
A string or `nil`.

Options
===
* `wmii2`

  **Required if wmii2 is used.**

* `wmii2_addline=<string>`

  E.g. `colors #000000 #ffffff #ff0000`.

* `bar=<character>`

  where `<character>` is either `l` or `r`, select which bar to use: left one or right one.

* `address=<address>`

  Address, in the Plan 9 resource format, at which to connect to the wmii 9P server.
  Default is to use `WMII_ADDRESS` environment variable.
