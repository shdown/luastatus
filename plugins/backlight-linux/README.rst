.. :X-man-page-only: luastatus-plugin-backlight-linux
.. :X-man-page-only: ################################
.. :X-man-page-only:
.. :X-man-page-only: #############################################
.. :X-man-page-only: Linux-specific backlight plugin for luastatus
.. :X-man-page-only: #############################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin shows the display backlight level when it changes.

Functions
=========
The following functions are provided:

* ``widget(tbl)``

  Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with
  the following fields:

  **(required)**

  - ``cb``: function

    The callback that will be called with a display backlight level (a number from 0 to 1), or
    with ``nil``.

  **(optional)**

  - ``syspath``: string

    Path to the device directory, e.g.::

        /sys/devices/pci0000:00/0000:00:02.0/drm/card0/card0-eDP-1/intel_backlight

  - ``timeout``: number

    Backlight information is hidden after the timeout in seconds (default is 2).

  - ``event``

    The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
    description of ``widget.event`` field).
