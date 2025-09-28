.. :X-man-page-only: luastatus-plugin-disk-io-linux
.. :X-man-page-only: ##############################
.. :X-man-page-only:
.. :X-man-page-only: #################################################
.. :X-man-page-only: Linux-specific disk I/O rate plugin for luastatus
.. :X-man-page-only: #################################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin periodically polls Linux ``procfs`` and calculates
disk I/O rates for all disk devices.

Functions
=========
The following functions are provided:

* ``read_diskstats(old, divisor)``

  Reads current ``diskstats`` measurements and also produces an array with
  deltas (number of bytes read/written per second for each disk device).

  ``divisor`` must be the period, in seconds, that this function is called.
  The function will average ``read_bytes`` and ``written_bytes`` over this
  period.

  Returns ``new, deltas``, where ``new`` is the table with current measurements.
  ``old`` must be a table with previous measurements (or an empty table if this is
  the first call to this function).

  ``deltas`` is an array of tables with the following entries:

  - ``num_major`` (number): disk device's major number;

  - ``num_minor`` (number): disk device's minor number;

  - ``name`` (string): disk device's name, as specified in ``/proc/diskstats``;

  - ``read_bytes`` (number): number of bytes read per second, averaged over the last ``divisor`` seconds;

  - ``written_bytes`` (number): number of bytes written per second, averaged over the last ``divisor`` seconds.

  **NOTE:** ``read_bytes`` and ``written_bytes`` entries may be negative; this means an overflow happened in
  kernel's statistics. If any of these number is negative, it is recommended simply not to include this disk
  device in current output.

  In order to use this function, you are expected to maintain a table ``old``, initially empty,
  and pass it to ``read_diskstats`` as the first argument each time, then set it to the first
  return value. For example::

    local old = {}
    local period = 5

    -- each 'period' seconds:
        local new, deltas = plugin.diskstats(old, period)
        old = new
        -- somehow use 'deltas'

* ``widget(tbl)``

  Constructs a ``widget`` table required by luastatus.

  ``tbl`` is a table with the following fields:

  **(required)**

  - ``cb``: a function

    The callback function that will be called with current deltas.
    The argument is the same as the second return value (``deltas``) of ``read_diskstats`` (see above
    for description of ``read_diskstats`` function for more information).

  **(optional)**

  - ``period``: table

    Period, in seconds, to query updates. Defaults to 1.

  - ``event``

    The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
    description of ``widget.event`` field).
