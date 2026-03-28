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
This plugin is a "multiplexor": it allows you to spawn multiple "nested widgets" and
receive information from all of them.
It also can invoke the ``event`` functions of nested widgets.

Options
=======
The following options are supported:

* ``data_sources`` (**required**): table

  This must be a dictionary (a table with string keys) of *data source specifications*,

  Each entry is treated as follows: the key is the identifier of the data source, and the
  value is the *data source specification*.
  A *data source specification* is a string with a Lua program which defines ``LL_data_source``
  global variable. This specification is used to initialize a new **nested widget**.

  Just like with normal luastatus widgets, ``plugin`` and ``cb`` entries are required, ``opts`` entry is optional.
  These entries have the same meaning as with normal luastatus widgets.

  The ``event`` function of a data source can later be called by the "main" multiplexor widget that is being
  discussed.

  Here is an example of a *data source specification*::

    [[
        widget = {
            plugin = 'xtitle',
            opts = {
                extended_fmt = true,
            },
            cb = function(t)
                return t.instance or ''
            end,
        }
    ]]

  Currently, there is a limit on the number of data sources: there can be at most 64 different data sources.

  For nested widget's ``cb`` functions, only string and nil return values are supported.

* ``greet``: boolean

  Whether or not to call the callback with ``what="hello"`` before doing anything else.
  Defaults to false.

``cb`` argument
===============
A table with ``what`` field.

* If ``what`` is ``"hello"``, then the ``greet`` was enabled and the plugin is making
  this call before doing anything else. Defaults to false.

* If ``what`` is ``"update"``, then some nested widget's plugin has produces a value.
  In this case, ``updates`` field is also present and is a table that contains all the
  updates (note that only updates are specified, not the full snapshot).
  Its keys are identifiers of data sources, and values are as follows:

  + if this nested widget's ``cb`` has generated a string value: a string;

  + if this nested widget's ``cb`` has generated a nil value: ``false``;

  + some error occurred: an integer with error code (see below).

Error codes
-----------
The following error codes can be reported (as of now):

* 128: plugin's ``run()`` function for this nested widget has returned (due to a fatal error),
  there will be no events from it anymore;

* 129: Lua error occurred in this nested widget's ``cb`` function;

* 130: This nested widget's ``cb`` function returned something other than a string or nil.

Functions
=========
The following functions are provided:

* ``luastatus.plugin.call_event(ident, content)``

  Calls the ``event`` function of the data source with identifier ``ident`` (which must be a string)
  with string ``content``.

  On success, returns ``true``. On failure, returns ``false, err_msg`` (but see `Notes`_).

Notes
=====
If the nested widgets have not been spawned yet, ``call_event`` throws an error
instead of returning ``false, err_msg``.
This is only possible if the function is being invoked from an event handler.
So, if you use this function from an event handler, you should consider this possibility.
