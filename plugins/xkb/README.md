This plugin monitors current keyboard layout.

Options
===
* `display`: string

  Display to connect to. Default is to use `DISPLAY` environment variable.

* `device_id`: number

  Keyboard device ID (as shown by `xinput(1)`). Default is `XkbUseCoreKbd`.

`cb` argument
===
A table with the following entries:

  * `group`: string

  Group name (if number of group names reported by libxkb-file is sufficient).

  * `group_id`: number (in Lua 5.3, integer)

  Group ID (0, 1, 2, or 3).
