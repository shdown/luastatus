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

It is also able to estimate the time remaining to full charge/discharge.

Property naming schemes
=======================
Linux has once changed its naming scheme of some files in ``/sys/class/power_supply/<device>/``.
Changes include:

* ``charge_full`` renamed to ``energy_full``;

* ``charge_now`` renamed to ``energy_now``;

* ``current_now`` renamed to ``power_now``.

By default, this plugin uses the "new" naming scheme; to change that, call
``B.use_props_naming_scheme("old")``, where ``B`` is the module object.

Functions
=========
The following functions are provided:

* ``read_uevent(device)``

    Reads a key-value table from ``/sys/class/power_supply/<device>/uevent``.

    If it cannot be read, returns ``nil``.

* ``read_files(device, proplist)``

    Reads a key-value pairs from ``/sys/class/power_supply/<device>/<prop>`` for each ``<prop>`` in
    ``proplist``.

    If a property cannot be read, it is not included to the resulting table.

* ``use_props_naming_scheme(name)``

    Tells the module which property naming scheme to use; ``name`` is either ``"old"`` or ``"new"``.

* ``get_props_naming_scheme()``

    Returns a table representing the current property naming scheme. It has the following keys:

    - ``EF``: either ``"charge_full"`` or ``"energy_full"``;

    - ``EN``: either ``"charge_now"`` or ``"energy_now"``;

    - ``PN``: either ``"current_now"`` or ``"power_now"``.

* ``est_rem_time(props)``

    Estimates the time in hours remaining to full charge/discharge. ``props`` is a table with
    following keys (assuming ``S`` is a table returned by ``get_props_naming_scheme()``):

    * ``"status"``;

    * ``S.EF``;

    * ``S.EN``;

    * ``S.PN``.

* ``widget(tbl)``

    Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with the following
    fields:

    **(required)**

    - ``cb``: function

        The callback that will be called with a table with the following keys:

        + ``status``: string

            A string with battery status text, e.g. ``"Full"``, ``"Unknown"``, ``"Charging"``,
            ``"Discharging"``.

            Not present if the status cannot be read.

        + ``capacity``: string

            A string representing battery capacity percent.

            Not present if the capacity cannot be read.

        + ``rem_time``: number

            If applicable, time (usually in hours) remaining to full charge/discharge.

            (Only present if the ``est_rem_time`` option was set to ``true``.)

    **(optional)**

    - ``dev``: string

        Device directory name; default is ``"BAT0"``.

    - ``no_uevent``: boolean

        If ``true``, data will be read from individual property files, not from the ``uevent``
        file.

    - ``props_naming_scheme``: boolean

        If present, ``use_props_naming_scheme`` will be called with it before anything else is done.

    - ``est_rem_time``: boolean

        If ``true``, time remaining to full charge/discharge will be estimated and passed to
        ``cb`` as ``rem_time`` table entry.

    - ``timer_opts``: table

        Options for the underlying ``timer`` plugin.

    - ``event``

        The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
        description of ``widget.event`` field).
