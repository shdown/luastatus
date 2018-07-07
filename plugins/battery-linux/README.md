This derived plugin periodically polls Linux `sysfs` for the state of a battery.

It is also able to estimate time time remaining to full charge/discharge.

Property naming schemes
===
Linux has once changed its naming scheme of some files in
`/sys/class/power_supply/<device>/`. Changes include:
  - `charge_full` renamed to `energy_full`;
  - `charge_now` renamed to `energy_now`;
  - `current_now` renamed to `power_now`.

By default, this plugin uses the “new” naming scheme; to change that, call
`B.use_props_naming_scheme('old')`, where `B` is the module object.

Functions
===
  - `read_uevent(device)`

      Reads a key-value table off `/sys/class/power_supply/<device>/uevent`.

  - `read_files(device, proplist)`

      Reads a key-value table off `/sys/class/power_supply/<device>/<prop>` for
      each `<prop>` in `proplist`.

  - `use_props_naming_scheme(name)`

      Tells the module which property naming scheme to use; `name` is either
      `'old'` or `'new'`.

  - `get_props_naming_scheme()`

      Kokoko.

  - `est_rem_time(props)`

      Estimates the time in hours remaining to full charge/discharge.
      `props` is a table with following keys (assuming `S` is a table returned
      by `get_props_naming_scheme()`):

        * `"status"`;
        * `S.EF`;
        * `S.EN`;
        * `S.PN`.

  - `widget(tbl)`

      Constructs a `widget` table required by luastatus. `tbl` is a table with
      the following fields:

      **(required)**

      * `cb`

      **(optional)**

      * `dev`

      * `no_uevent`

      * `props_naming_scheme`

      * `est_rem_time`

      * `timer_opts`

      * `event`
