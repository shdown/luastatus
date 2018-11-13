.. :X-man-page-only: luastatus-plugin-timer
.. :X-man-page-only: ######################
.. :X-man-page-only:
.. :X-man-page-only: ##########################
.. :X-man-page-only: timer plugin for luastatus
.. :X-man-page-only: ##########################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin is a simple timer, plus a wake-up FIFO can be specified.

Options
=======
The following options are supported:

* ``period``: number

    A number of seconds to sleep before calling ``cb`` again. May be fractional. Defaults to 1.

* ``fifo``: string

    Path to an existent FIFO. The plugin does not create FIFO itself. To force a wake-up,
    ``touch(1)`` the FIFO, that is, open it for writing and then close.

``cb`` argument
===============
A string decribing why the function is called:

* if it is ``"hello"``, the function is called for the first time;

* if it is ``"timeout"``, the function hasn't been called for ``period`` seconds;

* if it is ``"fifo"``, the FIFO has been touched.

Functions
=========
The following functions are provided:

* ``push_period(seconds)``

    Changes the timer period for one iteration.
