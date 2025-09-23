.. :X-man-page-only: luastatus-plugin-cpu-freq-linux
.. :X-man-page-only: ###############################
.. :X-man-page-only:
.. :X-man-page-only: #################################################
.. :X-man-page-only: Linux-specific CPU frequency plugin for luastatus
.. :X-man-page-only: #################################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin periodically polls Linux ``sysfs`` for the current
frequency of each of the CPUs.

Functions
=========
The following functions are provided:

* ``get_freqs(data)``

  Returns either an array or nil.
  If the result is nil, no data is currently available (most likely reason for this
  is that the set of plugged-in CPUs has changed; if so, the subsequent calls will
  return non-nil).

  Each array entry corresponds to a subsequent CPU; it is a table with the following fields:

    * ``min`` (number): minimal frequency of this CPU;
    * ``max`` (number): maximal frequency of this CPU;
    * ``cur`` (number): current frequency of this CPU.

  All values are in KHz.

  In order to use this function, you are expected to maintain a table ``data``, initially empty,
  and pass it to ``get_freqs`` each time. The only thing the caller is allowed to do with this
  table is to set ``please_reload`` field to ``true``. If ``please_reload`` field is true when
  this function is called, the function will forcefully reload all information and reset
  ``please_reload`` field to ``nil``.

* ``widget(tbl[, data])``

  Constructs a ``widget`` table required by luastatus.

  If ``data`` is specified, it must be an empty table; you can set ``please_reload`` field
  in this table to force a full reload. After each reported event, this field is reset to
  ``nil``.

  ``tbl`` is a table with the following fields:

  **(required)**

  - ``cb``: a function

    The callback function that will be called with current CPU frequencies, or with ``nil``.
    The arument is the same as the return value of ``get_freqs`` (see above for description
    of ``get_freqs`` function for more information).

  **(optional)**

  - ``period``: a number

    The period, in secods, to pass to the underlying ``timer`` plugin.

  - ``event``

    The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
    description of ``widget.event`` field).
