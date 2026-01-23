.. :X-man-page-only: luastatus-plugin-network-linux
.. :X-man-page-only: ##############################
.. :X-man-page-only:
.. :X-man-page-only: #############################################
.. :X-man-page-only: Network plugin for luastatus (Linux-specific)
.. :X-man-page-only: #############################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors network routing and link updates.
It can report IP addresses used for outgoing connections by various network interfaces, information
about a wireless connection, and the speed of an Ethernet connection.

Options
=======
The following options are supported:

* ``ip``: boolean

  Whether to report IP addresses used for outgoing connections. Defaults to true.

* ``wireless``: boolean

  Whether to report wireless connection info. Defaults to false.

* ``ethernet``: boolean

  Whether to report Ethernet connection info. Defaults to false.

* ``new_overall_fmt``: boolean

  Whether to use *new overall format* (see documentation below).
  Defaults to false.

* ``new_ip_fmt``: boolean

  Whether to report IP (``ipv4`` and ``ipv6``) addresses as tables (as opposed to strings).
  Defaults to false.

* ``timeout``: number

  If specified and not negative, requery information and call ``cb`` every ``timeout`` seconds.

  Note that this is done on any routing/link update anyway, so this is only useful if you want to
  show the "volatile" properties of a wireless connection such as signal level, bitrate, and
  frequency.

* ``make_self_pipe``: boolean

  If true, the ``wake_up()`` (see the `Functions`_ section) function will be available. Defaults to
  false.

``cb`` argument
===============
If the list of network interfaces cannot be fetched:
* if new overall format is used, a table ``{what = "error"}``;
* if old overall format is used, ``nil``.

Otherwise (if there is no error):
* if new overall format is used, a table ``{what = reason, data = tbl}``, where ``reason`` is a
string (see `Fetching the "what"`_) and ``tbl`` is the main table;
* if old overall format is used, the main table itself (but, in this case, this table also has a metatable;
see `Fetching the "what"`_ section).

The main table is a table where keys are network interface names (e.g. ``wlan0`` or ``wlp1s0``) and values
are tables with the following entries (all are optional):

* ``ipv4``, ``ipv6`` (only if the ``ip`` option is enabled)

  - If ``new_ip_fmt`` option was set to true, arrays (tables with numeric keys) of strings;
    each element corresponds to a local IPv4/IPv6 address of the interface.

    (In an extremely unlikely and probably logically impossible event that the number of addresses
    exceeds maximum Lua array length, the array will be truncated and the last element will be ``false``,
    not a string, to indicate truncation.)

  - Otherwise, the value behind ``ipv4``/``ipv6`` key is a string corresponding to *some* local
    IPv4/IPv6 address of the interface.

* ``wireless``: table with following entries (only if the ``wireless`` option is enabled):

  - ``ssid``: string

    802.11 network service set identifier (also known as the "network name").

  - ``signal_dbm``: number

    Signal level, in dBm.
    Generally, -90 corresponds to the worst quality, and -20 to the best quality.

  - ``frequency``: number

    Radio frequency, in Hz.

  - ``bitrate``: number

    Bitrate, in units of 100 kbit/s.

* ``ethernet``: table with following entries (only if the ``ethernet`` option is enabled):

  - ``speed``: number

    Interface speed, in Mbits/s.

Fetching the "what"
================================

If new overall format is used, and the argument is a table ``t``,
then the reason why the callback has been called is simply stored in ``t.what``.

If old overall format is used, and the argument (the main table) is a table ``t``,
then the reason why the callback has been called can be fetched as follows::

    getmetatable(t).what

This value is either:

* ``"update"`` network routing/link update;
* ``"timeout"``: timeout;
* ``"self_pipe"``: the ``luastatus.plugin.wake_up()`` function has been called.

For old overall format, the reason this shamanistic ritual with a metatable is
needed is that the support for any data other than the list of networks has not
been planned for in advance, and ``what`` is a valid network interface name.

Functions
=========
The following functions are provided:

* ``luastatus.plugin.wake_up()``

  Forces a call to ``cb``.

  Only available if the ``make_self_pipe`` option was set to ``true``; otherwise, it throws an
  error.
