.. :X-man-page-only: luastatus-plugin-file-contents-linux
.. :X-man-page-only: ####################################
.. :X-man-page-only:
.. :X-man-page-only: #################################################
.. :X-man-page-only: Linux-specific file contents plugin for luastatus
.. :X-man-page-only: #################################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin monitors the content of a file.

Functions
=========
The following functions are provided:

* ``widget(tbl)``

    Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with the following
    fields:

    **(required)**

    * ``filename``: string

        Path to the file to monitor.

    * ``cb``: function

        The callback that will be called with ``filename`` opened for reading.

    **(optional)**

    * ``timeout``, ``flags``

        Better do not touch (or see the code).

    * ``event``

        The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
        description of ``widget.event`` field).
