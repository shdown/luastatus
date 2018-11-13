.. :X-man-page-only: luastatus-barlib-dwm
.. :X-man-page-only: #####################
.. :X-man-page-only:
.. :X-man-page-only: ##########################
.. :X-man-page-only: dwm barlib for luastatus
.. :X-man-page-only: ##########################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This barlib updates the name of the root window.

It joins all non-empty strings returned by widgets by a separator, which defaults to ``" | "``.

It does not provide functions and does not support events.

``cb`` return value
===================
Either of:

* a string

    An empty string hides the widget.

* an array of strings

    Equivalent to returning a string with all non-empty elements of the array joined by the
    separator.

* ``nil``

    Hides the widget.

Options
=======
The following options are supported:

* ``display=<name>``

    Set the name of a display to connect to. Default is to use ``DISPLAY`` environment variable.

* ``separator=<string>``

    Set the separator.
