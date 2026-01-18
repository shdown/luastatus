.. :X-man-page-only: luastatus-mem-usage-linux
.. :X-man-page-only: #########################
.. :X-man-page-only:
.. :X-man-page-only: ################################################
.. :X-man-page-only: Linux-specific memory usage plugin for luastatus
.. :X-man-page-only: ################################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7


Overview
========
This derived plugin periodically polls Linux ``procfs`` for memory usage.

Functions
=========
* ``get_usage()``

  Returns a table with two entries, ``avail`` and ``total``. Both are tables that have ``value``
  (a number) and ``unit`` (a string) entries; you can assume ``unit`` is always ``"kB"``.

* ``widget(tbl)``

  Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with the following
  fields:

  **(required)**

  - ``cb``: function

    The callback that will be called with a table, which the same as one returned by ``get_usage()``.

  **(optional)**

  - ``timer_opts``: table

    Options for the underlying ``timer`` plugin.

  - ``event``

    The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
    description of ``widget.event`` field).
