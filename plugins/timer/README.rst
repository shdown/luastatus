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
A string describing why the function is called:

* if it is ``"hello"``, the function is called for the first time;

* if it is ``"timeout"``, the function hasn't been called for ``period`` seconds;

* if it is ``"fifo"``, the FIFO has been touched.

Functions
=========
The following functions are provided:

* ``push_period(seconds)``

  Changes the timer period for one iteration.

The following functions are provided as a part of "procalive" function set.
These functions are available in plugins, including this one, that can be used
to watch the state of some process(es):

* ``is_ok, err_msg = luastatus.plugin.access(path)``

  Checks if a given path exists, as if with ``access(path, F_OK)``.
  If it does exist, returns ``true, nil``. If it does not, returns
  ``false, nil``. If an error occurs, returns ``false, err_msg``.

* ``file_type, err_msg = luastatus.plugin.stat(path)``

  Tries to get the type of the file at the given path. On success returns
  either of: ``"regular"``, ``"dir"`` (directory), ``"chardev"`` (character device),
  ``"blockdev"`` (block device), ``"fifo"``, ``"symlink"``, ``"socket"``, ``"other"``.
  On failure returns ``nil, err_msg``.

* ``arr, err_msg = luastatus.plugin.glob(pattern)``

  Performs glob expansion of ``pattern``.
  A glob is a wildcard pattern like ``/tmp/*.txt`` that can be applied as
  a filter to a list of existing files names. Supported expansions are
  ``*``, ``?`` and ``[...]``. Please refer to ``glob(7)`` for more information
  on wildcard patterns.

  Note also that the globbing is performed with ``GLOB_MARK`` flag, so that
  in output, directories have trailing slash appended to their name.

  Returns ``arr, nil`` on success, where ``arr`` is an array of strings; these
  are existing file names that matched the given pattern. The order is arbitrary.
  On failure, returns ``nil, err_msg``.

* ``is_alive = is_process_alive(pid)``

  Checks if a process with PID ``pid`` is currently alive. ``pid`` must be a number.
  Returns a boolean that indicates whether the process is alive.
