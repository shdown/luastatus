.. :X-man-page-only: luastatus-plugin-temperature-linux
.. :X-man-page-only: ##################################
.. :X-man-page-only:
.. :X-man-page-only: ######################################################
.. :X-man-page-only: Linux-specific temperature sensor plugin for luastatus
.. :X-man-page-only: ######################################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin periodically polls Linux ``sysfs`` for the current
readings of various temperature sensors.

Functions
=========
The following functions are provided:

* ``get_temps(data)``

  Returns either an array or nil.
  If the result is nil, no data is currently available (the most likely reason for this is
  that the set of temperature sensors has changed; if so, the subsequent calls will
  return non-nil).

  Each array entry corresponds to a subsequent temperature sensor; it is a table with the following fields:

  * ``kind`` (string): kind of this temperature sensor; either ``"thermal"`` (``/sys/class/thermal/thermal_zone*``
    sensors) or ``"hwmon"`` (``/sys/class/hwmon/*`` sensors).
  * ``name`` (string): name of this temperature sensor; it is either:
    - for ``"thermal"`` kind: ``"thermal_zone${i}"``, where ``${i}`` is the index of the thermal zone, or;
    - for ``"hwmon"`` kind: the contents of ``/sys/class/hwmon/hwmon${i}/name`` file, where ``${i}`` is the index of hwmon sensor.
  * ``path`` (string): full path to the sensor readings file.
  * ``value`` (number): current temperature, in Celsius degrees.

  The array is sorted first by kind (``"thermal"`` entries go first), then by sensor index (``${i}`` above).

  In order to use this function, you are expected to maintain a table ``data``, initially empty,
  and pass it to ``get_temps`` each time.

  The only things the caller is allowed to do with this table (either before the first call or after a call) are:

  - set ``please_reload`` field to ``true``; if ``please_reload`` field is true when
    this function is called, the function will forcefully reload all information and reset
    ``please_reload`` field to ``nil``.

  - set ``filter_func`` field to a function that takes two string arguments (kind and name), and returns
    a boolean indicating whether this sensor should be monitored. If not set, all sensors will be monitored.

* ``widget(tbl[, data])``

  Constructs a ``widget`` table required by luastatus.

  If ``data`` is specified, it must be an empty table; you can set ``please_reload`` field
  in this table to force a full reload. On each reported event, before a call to ``tbl.cb``,
  this field is reset to ``nil``.

  It is technically possible to set ``filter_func`` field of the ``data`` table to a filter function (see above),
  but it is recommended to set it via ``tbl.filter_func`` field (instead of ``data.filter_func``).

  ``tbl`` is a table with the following fields:

  **(required)**

  - ``cb``: a function

    The callback function that will be called with current readings of temperature sensors, or with ``nil``.
    The argument is the same as the return value of ``get_temps`` (see above for description
    of ``get_temps`` function for more information).

  **(optional)**

  - ``filter_func``: function

    This must be a function that takes two string arguments (kind and name; see the description of the
    ``get_temps`` function above for what they mean), and returns a boolean indicating whether this sensor
    should be monitored. If not specified, all sensors will be monitored.

  - ``timer_opts``: table

    Options for the underlying ``timer`` plugin.

  - ``event``

    The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
    description of ``widget.event`` field).
