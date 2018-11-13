.. :X-man-page-only: luastatus-plugin-cpu-usage-linux
.. :X-man-page-only: ################################
.. :X-man-page-only:
.. :X-man-page-only: #############################################
.. :X-man-page-only: Linux-specific CPU usage plugin for luastatus
.. :X-man-page-only: #############################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin periodically polls Linux ``sysfs`` for the rate of CPU usage.

Functions
=========
The following functions are provided:

* ``get_usage(cur, prev[, cpu])``

    Returns current CPU usage rate, or ``nil`` if the amount of data collected is insufficient yet.

    ``cpu``, if passed and not ``nil``, specifies the CPU number, starting with 1. Otherwise,
    average rate will be calculated.

    In order to use this function, you are expected to maintain two tables, ``cur`` and ``prev``,
    both initially empty. After calling the function, you need to swap them, i.e. run
    ``cur, prev = prev, cur``.

    This function must be invoked each second.

* ``widget(tbl)``

    Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with the following
    fields:

    **(required)**

    - ``cb``: a function

        The callback function that will be called with current CPU usage rate, or with ``nil``.

    **(optional)**

    - ``cpu``: a number

        CPU number, starting with 1.

    - ``event``

        The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
        description of ``widget.event`` field).
