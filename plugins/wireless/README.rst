.. :X-man-page-only: luastatus-plugin-wireless
.. :X-man-page-only: #########################
.. :X-man-page-only:
.. :X-man-page-only: #############################################
.. :X-man-page-only: Wireless connection info plugin for luastatus
.. :X-man-page-only: #############################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin reports the information on a wireless (Wi-Fi) connection.

Options
=======
The following options are supported:

* ``iface``: string

    Network interface name (typically something like ``wlan0``, or ``wlp1s0`` if systemd is used).
    Run ``ls /sys/class/net`` to list available network interfaces.

    Default is to auto-detect wireless interface.

* ``timeout``: number

    If specified and not negative, requery information and call ``cb`` every ``timeout`` seconds.

    Note that this is done on any routing/link update anyway, so this is only useful if you want to
    show the "volatile" properties such as signal level, bitrate, and frequency.

``cb`` argument
===============
A table with ``what`` entry.

* If ``what`` is ``"update"``, the information on current wireless connection has been (re)queried,
  either because it is the first time the plugin does so, a routing/link update has been received,
  or a timeout has expired.
  Additionally, the following entries are provided (all are optional):

  - ``ssid``: string

    802.11 network service set identifier (also known as the “network name”).

  - ``signal_percent``: integer

    Signal level, in percents.

  - ``signal_dbm``: number

    Signal level, in dBms.
    Generally, -90 corresponds to the worst quality, and -20 to the best quality.

  - ``frequency``: number

    Radio frequency, in Hz.

  - ``bitrate``: number

    Bitrate, in units of 100 kbit/s.

* If ``what`` is ``"error"``, no data available for the interface.
