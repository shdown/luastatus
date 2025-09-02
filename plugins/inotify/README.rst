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

* ``ok, err = luastatus.plugin.access(path)``

    Tries to check if a file or directory at specified path exists (as if with ``access(..., F_OK)```
    system call). ``path`` must be a string with a path to file or directory.

    Returns ``true, nil`` if it exists, ``false, nil`` if it does not exist, ``false, err_msg``
    if the check for existence failed in some other reason; ``err_msg`` is a string with error message.

    This is useful in the following situation: suppose that the purpose of the widget is in checking
    whether a certain directory exists. It uses this plugin to poll for changes, and when it sees a
    change, it needs to check whether the directory currently exists; with Lua's standard library you
    can only check if a *file* exists, not a *directory*.

* ``luastatus.plugin.push_timeout(seconds)``

    Changes the timeout for one iteration.

Events and flag names
=====================
Each ``IN_*`` constant defined in ``<sys/inotify.h>`` corresponds to a string obtained from its name
by dropping the initial ``IN_`` and making the rest lower-case, e.g. ``IN_CLOSE_WRITE`` corresponds
to ``"close_write"``.

See ``inotify(7)`` for details.
