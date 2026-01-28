.. :X-man-page-only: luastatus-plugin-network-rate-linux
.. :X-man-page-only: ###################################
.. :X-man-page-only:
.. :X-man-page-only: ################################################
.. :X-man-page-only: Linux-specific network rate plugin for luastatus
.. :X-man-page-only: ################################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin periodically polls Linux ``procfs`` for the network receive/send rate (traffic
usage per unit of time).

Functions
=========
The following functions are provided:

* ``reader_make([iface_filter])``

  Creates a *reader* that can be used to query network rates for network interfaces that match
  the filter.

  If specified, ``iface_filter`` must be a function that takes a string (interface name) and
  return ``true`` if the reader should should report network rates for this interface,
  ``false`` or ``nil`` otherwise. If ``iface_filter`` is not specified the reader will report
  network rates for all interfaces.

* ``reader_read(reader, divisor[, in_array_form])``

  Queries network rates using the given reader. ``reader`` must be a reader instance returned
  by ``reader_make``.

  For a given interface, we define a *datum* as a table of form with keys ``"R"`` and ``"S"``;
  the ``"R"``/``"S"`` fields contain the increase in the number of bytes received/sent,
  correspondingly, with this interface, since the last query with this reader, divided by
  ``divisor``.

  Normally, ``divisor`` should be the period with which you query the network rates with this
  reader; if not applicable, set ``divisor`` to 1 and interpret the values yourself.

  If ``in_array_form`` is ``true``, the result will be an array (a table with numeric keys) of
  ``{iface_name, datum}`` entries, in the same order as reported by the kernel. Otherwise (if
  ``in_array_form`` is not specified or ``false``), the result will be a dictionary (a table
  with string keys) with interface names as keys and datums as values.

* ``widget(tbl)``

  Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with the following
  fields:

  **(required)**

  - ``cb``: a function

    The callback to call each ``period`` (see below) seconds with the network rate data.
    It is called with a single argument that is equivalent to ``reader_read`` return
    value (please refer to the description of that function above).

  **(optional)**

  Note: ``iface_filter``, ``iface_only`` and ``iface_except`` are mutually exclusive.
  If neither is specified, network rates for all interfaces will be reported.

  - ``iface_filter``: a function

    If specified, this must be a function that takes a string (interface name) and return
    ``true`` if we are interested in network rates for this interface, ``false`` or ``nil``
    otherwise.

  - ``iface_only``/``iface_except``: a string or a table

    These options can be used to specify a whitelist/blacklist, correspondingly, for network
    interfaces that we are interested in.

    If it is a string, it will be interpreted as an interface name; only interface with this
    name will be whitelisted/blacklisted.

    If it is a table with numeric keys, it will be interpreted as a list of interface names
    that should be whitelisted/blacklisted.

    If it is a table with string keys, it will be interpreted as a lookup table:
    interface names that correspond to a truth-y (anything except ``false`` and ``nil``)
    value in this table will be whitelisted/blacklisted.

    If it is an empty table, no interfaces will be whitelisted/blacklisted.

  - ``in_array_form``: a boolean

    Please refer to the description of ``in_array_form`` argument to ``reader_read``
    function above.

    Defaults to ``false``.

  - ``period``: a number

    The period, in seconds, with which the callback function will be called.

    This will also be a divisor (all rates will be divided by this value
    so that the rates be per ``period`` seconds instead of 1 second). Please
    refer to the description of ``divisor`` argument to ``reader_read`` function
    above for more information.

    Defaults to 1.

  - ``event``

    The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
    description of ``widget.event`` field).
