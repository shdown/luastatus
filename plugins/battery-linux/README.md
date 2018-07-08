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
`B.use_props_naming_scheme("old")`, where `B` is the module object.

Functions
===
  - `read_uevent(device)`

      Reads a key-value table from `/sys/class/power_supply/<device>/uevent`.

      If it cannot be read, returns `nil`.

  - `read_files(device, proplist)`

      Reads a key-value pairs from `/sys/class/power_supply/<device>/<prop>` for
      each `<prop>` in `proplist`.

      If a property cannot be read, it is not included to the resulting table.

  - `use_props_naming_scheme(name)`

      Tells the module which property naming scheme to use; `name` is either
      `"old"` or `"new"`.

  - `get_props_naming_scheme()`

      Returns a table representing the current property naming scheme.
      It has the following keys:

      * `EF`: either `"charge_full"` or `"energy_full"`;
      * `EN`: either `"charge_now"` or `"energy_now"`;
      * `PN`: either `"current_now"` or `"power_now"`.

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

      * `cb`: a callback function that will be called with a table with the
      following keys:

        - `status`: either `"Full"`, `"Unknown"`, `"Charging"`, `"Discharging"`,
        or `nil` if cannot be read;
        - `capacity`: a string representing battery capacity percent (or `nil`);
        - `rem_time`: if applicable, time remaining to full charge/discharge
        (only present if `est_rem_time` was set to `true`);

      **(optional)**

      * `dev`: device directory name; default is `"BAT0"`;

      * `no_uevent`: if `true`, data will be read from individual property files
      and not from the `uevent` file;

      * `props_naming_scheme`: if present, `use_props_naming_scheme` will be
      called with it before anything else is done;

      * `est_rem_time`: if `true`, time remaining to full charge/discharge will
      be estimated and passed to `cb` as `rem_time` table entry;

      * `timer_opts`: options for the underlying `timer` plugin;

      * `event`: event entry of the resulting table.
