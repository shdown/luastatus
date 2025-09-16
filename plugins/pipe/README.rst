.. :X-man-page-only: luastatus-plugin-pipe
.. :X-man-page-only: #####################
.. :X-man-page-only:
.. :X-man-page-only: ##########################################
.. :X-man-page-only: process output reader plugin for luastatus
.. :X-man-page-only: ##########################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7


Overview
========
This derived plugin monitors the output of a process and calls the callback function whenever it
produces a line.

Functions
=========
The following functions are provided:

* ``shell_escape(x)``

  If ``x`` is a string, escapes it as a shell argument; if ``x`` is an array of strings, escapes
  them and then joins by space.

* ``widget(tbl)``

  Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with the following
  fields:

  **(required)**

  - ``command``: string

    ``/bin/sh`` command to spawn.

  - ``cb``: function

    The callback that will be called with a line produced by the spawned process each time one
    is available.

  **(optional)**

  - ``on_eof``: function

    Callback to be called when the spawned process closes its stdout. Default is to simply
    hang to prevent busy-looping forever.

  - ``event``

    The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
    description of ``widget.event`` field).
