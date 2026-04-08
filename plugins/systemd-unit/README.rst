.. :X-man-page-only: luastatus-plugin-systemd-unit
.. :X-man-page-only: #############################
.. :X-man-page-only:
.. :X-man-page-only: ##################################################
.. :X-man-page-only: Systemd unit state monitoring plugin for luastatus
.. :X-man-page-only: ##################################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========

This derived plugin monitors the state of a systemd unit.

Functions
=========

The following functions are provided:

* ``subscribe()``

  Subscribes to the systemd D-Bus manager to receive unit state change notifications.

* ``get_unit_path_by_unit_name(unit_name)``

  Returns the D-Bus object path corresponding to the specified systemd unit name.

* ``get_state_by_unit_path(unit_path)``

  Returns the ``ActiveState`` property of the unit located at the given D-Bus object path.

* ``get_state_by_unit_name(unit_name)``

  Returns the current ``ActiveState`` propety for the specified systemd unit name.
  This is just a combination of ``get_state_by_unit_path`` and ``get_unit_path_by_unit_name``.

* ``widget(tbl)``

  Constructs a ``widget`` table required by luastatus. ``tbl`` must be a table with the following fields:

  **(required)**
  - ``unit_name``: string

    The name of the systemd unit to monitor (e.g., ``"tor.service"``).

  - ``cb``: function

    The callback function that will be called with the current unit state as a string.

  **(optional)**

  - ``no_throw``: boolean

    If set to ``true``, errors encountered while fetching the unit state will be caught,
    a warning will be printed to the console, and ``cb`` will be called with ``nil``
    instead of propagating the error. Defaults to ``false``.

  - ``forced_refresh_interval``: number

    The timeout interval in seconds for forced state checks when D-Bus signals are not
    received. Defaults to ``30``.

  - ``event``

    The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
    description of ``widget.event`` field).
