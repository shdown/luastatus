.. :X-man-page-only: luastatus-plugin-pipev2
.. :X-man-page-only: #######################
.. :X-man-page-only:
.. :X-man-page-only: ##############################
.. :X-man-page-only: pipe plugin for luastatus (v2)
.. :X-man-page-only: ##############################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin spawns a process and then reads lines (or chunks separated by some other configured delimiter) from its stdout.

It can optionally redirect the child process' stdin as well.
This way, a widget is able to write data into its stdin, thus establishing bidirectional communication.

It can also send signals to the child process.

Options
=======
The following options are supported:

* ``argv``: array of string (**required**)

  Program name or path, followed by the arguments.

* ``delimiter``: string

  Delimiter to use for breaking stream of bytes into chunks.
  Defaults to ``\n``, which means report text lines.
  If specified, must be exactly one byte long.

* ``pipe_stdin``: boolean

  Whether or not to rediect the child process' stdin.
  If enabled, the widget will be able to write data into its stdin, thus establishing bidirectional communication.
  Defaults to ``false``.

* ``greet``: boolean

  Whether or not to call the callback in the beginning with ``what="hello"``.

* ``bye``: boolean

  Whether or not to call the callback in the end (after the process has died) with ``what="bye"``.

``cb`` argument
===============
A table with ``what`` key:

* If it's ``"hello"``, then the ``greet`` option was enabled, and the callback is called before any other calls.

* If it's ``"line"``, then the child process has just produced a line (or, if a custom delimiter is used, a chunk separated by the delimiter).
  The ``line`` field of the table contains the line itself.

* If it's ``"bye"``, then the ``bye`` option was enabled, and the child process has just died.

Functions
=========
This plugin provides the following functions:

* ``luastatus.plugin.write_to_stdin(bytes)``

  Writes ``bytes``, which must be a string, into the child process' stdin.
  Only available if ``pipe_stdin`` option was enabled.

  On success, it returns ``true``.
  On failure, it returns ``false, err_msg``, where ``err_msg`` is the error message,
  but see `Notes`_.

* ``luastatus.plugin.kill([signal])``

  Sends a signal to the child process.
  If ``signal`` is not passed or is ``nil``, SIGTERM is implied.
  ``signal`` can be either a string with the spelling of a signal (e.g. ``"SIGUSR1"``)
  or a signal number (e.g. 9).

  On success, it returns ``true``.
  On failure, it returns ``false, err_msg``, where ``err_msg`` is the error message,
  but see `Notes`_.

  For the list of supported signal spellings, see `Supported signal names`_.

* ``luastatus.plugin.get_sigrt_bounds()``

  Returns ``SIGRTMIN, SIGRTMAX``. Might be useful if you want to send real-time signals
  and don't want to depend on OS/architecture.

Supported signal names
----------------------
All signals defined by POSIX.1-2008 (without extensions) are supported:
SIGABRT, SIGALRM, SIGBUS, SIGCHLD, SIGCONT, SIGFPE, SIGHUP, SIGILL, SIGINT, SIGKILL, SIGPIPE,
SIGQUIT, SIGSEGV, SIGSTOP, SIGTERM, SIGTSTP, SIGTTIN, SIGTTOU, SIGUSR1, SIGUSR2, SIGURG.

Notes
-----
If the child process has not been spawned yet, ``write_to_stdin`` and ``kill`` throw an error
instead of returning ``false, err_msg``.
This is only possible if the function is being invoked from an event handler.
So, if you use this function from an event handler, you should consider this possibility.
