This derived plugin shows display backlight level when it changes.

Functions
===

* `widget(tbl)`

    Constructs a `widget` table required by luastatus. `tbl` is a table with
    the following fields:

    **(required)**

    - `cb`: a callback function that will be called with a display backlight level (a number from 0 to 1), or with `nil`;

    **(optional)**

    - `syspath`: path to the device directory, e.g. `/sys/devices/pci0000:00/0000:00:02.0/drm/card0/card0-eDP-1/intel_backlight/brightness`.

    - `timeout`: backlight infomation show timeout in seconds (default is 2);

    - `event`: event entry of the resulting table.
