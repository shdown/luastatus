.. :X-man-page-only: luastatus-barlib-stdout
.. :X-man-page-only: #######################
.. :X-man-page-only:
.. :X-man-page-only: ###########################
.. :X-man-page-only: stdout barlib for luastatus
.. :X-man-page-only: ###########################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This barlib simply writes lines to a file descriptor.

It can be used for status bars such as **dzen**/**dzen2**, **xmobar**, **yabar**, **dvtm**, and
others.

It joins all non-empty strings returned by widgets by a separator, which defaults to ``" | "``.

It does not provide functions and does not support events.

Redirections and ``luastatus-stdout-wrapper``
=============================================
Since we need to write to stdout, it is very easy to mess things up: Lua's ``print()`` prints to
stdout, processes spawned by widgets/plugins inherit our stdin and stdout, etc.

That's why this barlib requires that stdout file descriptor is manually redirected. A shell wrapper,
``luastatus-stdout-wrapper``, is shipped with it; it does all the redirections needed and executes
``luastatus`` with ``-b stdout`` and additional arguments passed by you.

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

* ``out_fd=<fd>``

   File descriptor to write to. Usually set by the wrapper.

* ``separator=<string>``

   Set the separator.

* ``error=<string>``

   Set the content of an "error" segment. Defaults to ``"(Error)"``.
