.. :X-man-page-only: luastatus-plugin-xtitle
.. :X-man-page-only: #######################
.. :X-man-page-only:
.. :X-man-page-only: ########################################
.. :X-man-page-only: active window title plugin for luastatus
.. :X-man-page-only: ########################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors the title of the active window.

Options
=======

* ``display``: string

    Display to connect to. Default is to use ``DISPLAY`` environment variable.

* ``visible``: boolean

    If true, try to retrieve the title from the ``_NET_WM_VISIBLE_NAME`` atom. Default is false.

* ``extended_fmt``: boolean

    If true, *class* and *instance* of the active window will also be reported;
    the argument to ``cb`` will be a table instead of string or nil.
    Defaults to false.

``cb`` argument
===============
If ``extended_fmt`` option was not enabled (this is the default), the argument is a string
with the title of the active window, or ``nil`` if there is no active window.

If ``extended_fmt`` is enabled, the argument will be a table with keys
``class``, ``instance`` and ``title``. All values are strings or ``nil`` if there is no active window.
