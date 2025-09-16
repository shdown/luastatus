.. :X-man-page-only: luastatus-plugin-is-program-running
.. :X-man-page-only: ###################################
.. :X-man-page-only:
.. :X-man-page-only: #########################################################
.. :X-man-page-only: Plugin for luastatus which checks if a program is running
.. :X-man-page-only: #########################################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin checks whether a certain program is running, although
in order to this plugin to work, the program should indicate its state
via one of the following widespread mechanisms:

  1. PID file;
  2. Creation of a file. The file must have a fixed path;
  3. Creation of a file (with any name) in an otherwise-empty directory. The directory
     must have a fixed path.

Functions
=========
The following functions are provided:

* ``make_watcher(kind, path)``

  Returns a watcher; a watcher is a function that takes no argument and returns a single
  boolean value indicating whether the program is running.

  Kind must be either ``"pidfile"``, ``"file_exists"``, ``"dir_nonempty"`` or ``"dir_nonempty_with_hidden"``:
  * ``"pidfile"`` means check by PID file located at path ``path``;
  * ``"file_exists"`` means check by existence of a file or directory at path ``path``;
  * ``"dir_nonempty"`` means check by whether a directory located at path ``path`` is non-empty (**excluding** hidden files);
  * ``"dir_nonempty_with_hidden"`` means check by whether a directory located at path ``path`` is non-empty (**including** hidden files).

* ``widget(tbl)``

  Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with the following
  fields:

    **(required)**

    - ``cb``: a function

      The callback function that will be called with current CPU usage rate, or with ``nil``.

    **(exactly one of the following is required)**

    - ``kind`` and ``path``: strings

      Please refer to the documentation for the ``make_watcher`` function above for
      semantics of ``kind`` and ``path``.
      If ``kind`` and ``path`` are specified, the plugin will only monitor state of a
      single program, and ``cb`` will be called with a boolean argument.

    - ``many``: array

      If specified, the plugin will monitor states of N programs, where N is the length
      of the ``many`` array. The array should contain tables (each being a watcher
      specification) with the following entries:

      * ``id``: string

        A string used to identify this entry in the ``many`` array;

      * ``kind`` and ``path``: strings

        Please refer to the documentation for the ``make_watcher`` function above for
        semantics of ``kind`` and ``path``.

      If ``many`` is specified, ``cb`` will be called with a table argument in which
      keys are identification strings (``id`` entry in the watcher specification;
      see above), and values are booleans indicating whether the corresponding
      process is running.

    **(optional)**

    - ``timer_opts``

      Options to pass to the underlying ``timer`` plugin. Note that this includes the period
      with which this plugin will check if the program(s) is running.
      The period of the ``timer`` plugin defaults to 1 second.

    - ``event``

      The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
      description of ``widget.event`` field).
