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

It does not provide functions.

Redirections and ``luastatus-stdout-wrapper``
=============================================
Since we need to write to stdout, it is very easy to mess things up: Lua's ``print()`` prints to
stdout, processes spawned by widgets/plugins inherit our stdin and stdout, etc.

That's why this barlib requires that stdout file descriptor is manually redirected. A shell wrapper,
``luastatus-stdout-wrapper``, is shipped with it; it does all the redirections needed and executes
``luastatus`` with ``-b stdout`` and additional arguments passed by you.

It does not redirect stdin and/or pass ``in_fd=`` option. Make your own wrapper if this is needed.

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

* ``in_filename=<string>``
* ``in_fd=<fd>``

  Enable event watcher.

  If ``in_filename=<string>`` is specified, this barlib will open the specified
  filename for reading and then read from. If ``in_fd=<fd>`` is specified, the
  specified file descriptor will be read from. It is invalid to pass both options;
  in this case, this barlib will fail to initialize.

  This barilib will then read lines from the specified source. Each line will be
  treated as an event.

  This barlib doesn't try to interpret the content of the line; instead, the event
  will be broadcast to all widgets. Thus, if this option is used, it is the
  responsibility of each widget (that listens to the events) to check if the event
  is somehow related to it.

``event`` argument
==================
Events are only reported if either ``in_filename=`` or ``in_fd=`` option was
specified (see above). In this case, the argument is the line that was read from
the specified file, without the trailing newline character.
