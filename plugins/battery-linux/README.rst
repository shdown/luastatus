.. :X-man-page-only: luastatus-plugin-battery-linux
.. :X-man-page-only: ##############################
.. :X-man-page-only:
.. :X-man-page-only: ###########################################
.. :X-man-page-only: Linux-specific battery plugin for luastatus
.. :X-man-page-only: ###########################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin periodically polls Linux ``sysfs`` for the state of a battery.

It is also able to estimate the time remaining to full charge/discharge and
the current battery consumption rate.

Functions
=========
The following functions are provided:

* ``widget(tbl)``

  Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with the following
  fields:

  **(required)**

  - ``cb``: function

    The callback that will be called with a table with the following keys:

    + ``status``: string

      A string with battery status text, e.g. ``"Full"``, ``"Unknown"``, ``"Charging"``, ``"Discharging"``.

      Not present if the status cannot be read (for example, when the battery is missing).

    + ``capacity``: number

      A percentage representing battery capacity.

      Not present if the capacity cannot be read.

    + ``rem_time``: number

      Time (in hours) remaining to full charge/discharge.

      Usually only present on battery charge/discharge.

    + ``consumption``: number

      The current battery consumption/charge rate in watts.

      Usually only present on battery charge/discharge.

    **(optional)**

    - ``dev``: string

      Device directory name under ``/sys/class/power_supply``; default is ``"BAT0"``.

    - ``period``: number

      The period in seconds for polling ``sysfs``; default is 2 seconds.

      Because this plugin utilizes ``udev`` under the hood, it is able to respond to
      battery state changes (such as cable (un-)plugging) immediately, irrespective of
      the value of the period.

    - ``use_energy_full_design``: boolean

      If ``true``, the ``energy_full_design`` property (not ``energy_full``) will be used for
      capacity calculation.

    - ``event``

      The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
      description of ``widget.event`` field).
