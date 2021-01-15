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

* ``luastatus.plugin.push_timeout(seconds)``

    Changes the timeout for one iteration.

Events and flag names
=====================
Each ``IN_*`` constant defined in ``<sys/inotify.h>`` corresponds to a string obtained from its name
by dropping the initial ``IN_`` and making the rest lower-case, e.g. ``IN_CLOSE_WRITE`` corresponds
to ``"close_write"``.

See ``inotify(7)`` for details.
