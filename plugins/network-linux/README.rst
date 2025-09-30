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
about a wireless connection, and the speed of an ethernet connection.

Options
=======
The following options are supported:

* ``ip``: boolean

  Whether to report IP addresses used for outgoing connections. Defaults to true.

* ``wireless``: boolean

  Whether to report wireless connection info. Defaults to false.

* ``ethernet``: boolean

  Whether to report ethernet connection info. Defaults to false.

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
If the list of network interfaces cannot be fetched, ``nil``.

Otherwise, a table where keys are network interface names (e.g. ``wlan0`` or ``wlp1s0``) and values
are tables with the following entries (all are optional):

* ``ipv4``, ``ipv6`` (only if the ``ip`` option is enabled)

  - If ``new_ip_fmt`` option was set to true, arrays (tables with numeric keys) of strings;
    each element corresponds to a local IPv4/IPv6 address of the interface.

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

Fetching the reason why ``cb`` was called
=========================================
If the argument is a table ``t``, the reason why the callback has been called can be fetched as follows::

    getmetatable(t).what

This value is either:

* ``"update"`` network routing/link update;
* ``"timeout"``: timeout;
* ``"self_pipe"``: the ``luastatus.plugin.wake_up()`` function has been called.

The reason this shamanistic ritual with a metatable is needed is that the support for any
data other than the list of networks has not been planned for in advance, and ``what``
is a valid network interface name.

Functions
=========
The following functions are provided:

* ``luastatus.plugin.wake_up()``

  Forces a call to ``cb``.

  Only available if the ``make_self_pipe`` option was set to ``true``; otherwise, it throws an
  error.
