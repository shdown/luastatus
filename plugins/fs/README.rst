.. :X-man-page-only: luastatus-plugin-fs
.. :X-man-page-only: #####################
.. :X-man-page-only:
.. :X-man-page-only: ###############################
.. :X-man-page-only: disk usage plugin for luastatus
.. :X-man-page-only: ###############################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors file system usage. It is timer-driven, plus a wake-up FIFO can be specified.

Options
========
The following options are supported:

* ``paths``: array of strings

  For each of these paths, an information on usage of the file system it belongs to is returned.

* ``globs``: array of strings

  Same as ``paths`` but accepts glob patterns. It is not an error if the pattern expands to
  nothing. Useful for monitoring filesystems which are mounted at runtime.

* ``enable_dyn_paths``: boolean

  Whether to enable support for adding/removing paths dynamically (in run time).
  If set to true, functions ``add_dyn_path``, ``remove_dyn_path``, ``get_max_dyn_paths``
  will be available.

  Defaults to false.

* ``period``: number

  A number of seconds to sleep before calling ``cb`` again. May be fractional. Defaults to 10.

* ``fifo``: string

  Path to an existent FIFO. The plugin does not create FIFO itself. To force a wake-up,
  ``touch(1)`` the FIFO, that is, open it for writing and then close.

``cb`` argument
===============
A table where keys are paths and values are tables with the following entries:

* ``total``: number of bytes total;

* ``free``: number of bytes free;

* ``avail``: number of bytes free for unprivileged users.

Functions
=========
If ``enable_dyn_paths`` option was set to true, then:

* the set of "dynamic" path, initially empty, is maintained in run time;

* each call to ``cb`` also includes all paths from this set;

* the functions related to adding/removing paths dynamically are provided.

The following functions that are related to adding/removing dynamically are provided
if ``enable_dyn_paths`` option was set to true:

* ``luastatus.plugin.add_dyn_path(path)``

  Tries to add ``path`` (which must be a string) to the set of "dynamic" paths.
  On successful insertion, it returns ``true``.
  If the path is already in the set, returns ``false``.
  If the path is not in the set, but the set is already of maximum possible size, it throws an error.

* ``luastatus.plugin.remove_dyn_path(path)``

  Tries to remove ``path`` (which must be string) from the set of "dynamic" paths.
  On successful removal, it returns ``true``.
  If the path is not in the set, returns ``false``.

* ``luastatus.plugin.get_max_dyn_paths()``

  Returns an integer indicating the maximum possible size of the set of "dynamic" paths (currently 256).
