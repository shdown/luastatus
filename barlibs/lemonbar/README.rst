.. :X-man-page-only: luastatus-barlib-lemonbar
.. :X-man-page-only: #########################
.. :X-man-page-only:
.. :X-man-page-only: #############################
.. :X-man-page-only: lemonbar barlib for luastatus
.. :X-man-page-only: #############################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Warning
=======
``lemonbar`` has a critical bug that will likely spoil your impression of using luastatus with it.
For details, please see:

* https://github.com/LemonBoy/bar/pull/198

* https://github.com/LemonBoy/bar/issues/107

* https://github.com/shdown/luastatus/issues/12#issuecomment-277616761 and down the thread.

Please use ``lemonbar`` from

   https://github.com/shdown/bar/tree/buffer-bug-fix,

or simply apply the
following patch:

   https://github.com/shdown/bar/commit/eb0e9e79b9a35c3093f6e2e90d9fbd175a2305ef.patch

Overview
========
This barlib talks with ``lemonbar``.

It joins all non-empty strings returned by widgets by a separator, which defaults to ``" | "``.

Redirections and ``luastatus-lemonbar-launcher``
================================================
``lemonbar`` is not capable of creating a bidirectional pipe itself; instead, it requires all the
data to be written to its stdin and read from its stdout.

That's why the input/output file descriptors of this barlib must be manually redirected.

A launcher, ``luastatus-lemonbar-launcher``, is shipped with it; it spawns ``lemonbar`` connected to
a bidirectional pipe and executes ``luastatus`` with ``-b lemonbar``, all the required ``-B``
options, and additional arguments passed by you.

Pass each ``lemonbar`` argument with ``-p``, then pass ``--``, then pass luastatus arguments, e.g.::

   luastatus-lemonbar-launcher -p -B#111111 -p -f'Droid Sans Mono for Powerline:pixelsize=12:weight=Bold' -- -Bseparator=' ' widget1.lua widget2.lua

``cb`` return value
===================
Either of:

* a string with lemonbar markup

    An empty string hides the widget.

* an array of such strings

    Equivalent to returning a string with all non-empty elements of the array joined by the
    separator.

* ``nil``

    Hides the widget.

Functions
=========
The following functions are provided:

* ``escaped_str = luastatus.barlib.escape(str)``

    Escapes text for lemonbar markup.

``event`` argument
==================
A string with the name of the command.

Options
=======
The following options are supported:

* ``in_fd=<fd>``

    File descriptor to read ``lemonbar`` input from. Usually set by the launcher.

* ``out_fd=<fd>``

    File descriptor to write to. Usually set by the launcher.

* ``separator=<string>``

    Set the separator.
