.. :X-man-page-only: luastatus-plugin-fs
.. :X-man-page-only: #####################
.. :X-man-page-only:
.. :X-man-page-only: ###############################
.. :X-man-page-only: disk usage plugin for luastatus
.. :X-man-page-only: ###############################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors file system usage. It is timer-driven, plus a wake-up FIFO can be specified.

Options
========
The following options are supported:

* ``paths``: array of strings

    For each of these paths, an information on usage of the file system it belongs to is returned.

* ``period``: number

    A number of seconds to sleep before calling ``cb`` again. May be fractional. Defaults to 10.

* ``fifo``: string

    Path to an existent FIFO. The plugin does not create FIFO itself. To force a wake-up,
    ``touch(1)`` the FIFO, that is, open it for writing and then close.

``cb`` argument
===============
A table where keys are paths and values are tables with the following entries:

* ``total``: number of bytes total;

* ``free``: number of bytes free;

* ``avail``: number of bytes free for unprivileged users.
