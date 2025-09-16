.. :X-man-page-only: luastatus-plugin-inotify
.. :X-man-page-only: ########################
.. :X-man-page-only:
.. :X-man-page-only: ############################
.. :X-man-page-only: inotify plugin for luastatus
.. :X-man-page-only: ############################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors inotify events.

Options
=======
* ``watch``: table

  A table in which keys are the paths to watch and values are the tables with event names,
  for example, ``{["/home/user"] = {"create", "delete", "move"}}`` (see the
  `Events and flag names`_ section).

* ``greet``: boolean

  Whether or not to call ``cb`` with ``what="hello"`` as soon as the widget starts. Defaults to
  false.

* ``timeout``: number

  If specified and not negative, this plugin calls ``cb`` with ``what="timeout"`` if no event has
  occurred in ``timeout`` seconds.

``cb`` argument
===============
A table with ``what`` entry.

* If ``what`` is ``"hello"``, the function is being called for the first time (and the ``greet``
  option was set to ``true``).

* If ``what`` is ``"timeout"``, the function has not been called for the number of seconds specified
  as the ``timeout`` option.

* If ``what`` is ``"event"``, an inotify event has been read; in this case, the table has the
  following additional entries:

  - ``wd``: integer

    Watch descriptor (see the `Functions`_ section) of the event.

  - ``mask``: table

    For each event name or event flag (see the `Events and flag names`_ section), this table
    contains an entry with key equal to its name and ``true`` value.

  - ``cookie``: number

    Unique cookie associating related events (or, if there are no associated related events, a
    zero).

  - ``name``: string (optional)

    Present only when an event is returned for a file inside a watched directory; identifies the
    filename within the watched directory.

Functions
=========
Each file being watched is assigned a *watch descriptor*, which is a non-negative integer.

* ``wds = luastatus.plugin.get_initial_wds()``

  Returns a table that maps *initial* paths to their watch descriptors.

* ``wd = luastatus.plugin.add_watch(path, events)``

  Add a new file to watch. Returns a watch descriptor on success, or ``nil`` on failure.

* ``is_ok = luastatus.plugin.remove_watch(wd)``

  Removes a watch by its watch descriptor. Returns ``true`` on success, or ``false`` on failure.

* ``tbl = luastatus.plugin.get_supported_events()``

  Returns a table with all events supported by the plugin.

  In this table, keys are events names, values are strings which represent the mode of the event.
  The value is
  ``"i"`` for an input-only event (can only be listened to, never occurs in a returned event),
  ``"o"`` for an output-only event (only occurs in a returned event, cannot be listened to),
  ``"io"`` for an event that can be both listened to and occur in a returned event.

  This set depends on the version of glibc that the plugin has been compiled with, which is
  something a widget should not be required to know. By calling this function, the widget can
  query which events are supported by the plugin.

  Testing which events are actually supported by the Linux kernel that the system is running is a
  harder problem.
  The ``inotify(7)`` man page gives kernel versions in which certain events were introduced
  (e.g. ``IN_DONT_FOLLOW (since Linux 2.6.15)``).
  One should probably parse the output of ``uname -r`` command or the contents of
  ``/proc/sys/kernel/osrelease`` file in order to check for the version of the kernel.

* ``luastatus.plugin.push_timeout(seconds)``

  Changes the timeout for one iteration.

The following functions are provided as a part of "procalive" function set.
These functions are available in plugins, including this one, that can be used
to watch the state of some process(es):

* ``access(path)``

  Checks if a given path exists, as if with ``access(path, F_OK)``.
  If it does exist, returns ``true, nil``. If it does not, returns
  ``false, nil``. If an error occurs, returns ``false, err_msg``.

* ``stat(path)``

  Tries to get the type of the file at the given path. On success returns
  either of: ``"regular"``, ``"dir"`` (directory), ``"chardev"`` (character device),
  ``"blockdev"`` (block device), ``"fifo"``, ``"symlink"``, ``"socket"``, ``"other"``.
  On failure returns ``nil, err_msg``.

* ``glob(pattern)``

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

* ``is_process_alive(pid)``

  Checks if a process with PID ``pid`` is currently alive. ``pid`` must be a number.
  Returns a boolean that indicates whether the process is alive.

Events and flag names
=====================
Each ``IN_*`` constant defined in ``<sys/inotify.h>`` corresponds to a string obtained from its name
by dropping the initial ``IN_`` and making the rest lower-case, e.g. ``IN_CLOSE_WRITE`` corresponds
to ``"close_write"``.

See ``inotify(7)`` for details.
