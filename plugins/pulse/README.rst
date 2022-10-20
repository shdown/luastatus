.. :X-man-page-only: luastatus-plugin-pulse
.. :X-man-page-only: ######################
.. :X-man-page-only:
.. :X-man-page-only: ###############################
.. :X-man-page-only: PulseAudio plugin for luastatus
.. :X-man-page-only: ###############################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors the volume and mute status of a PulseAudio sink.

Options
=======
The following options are supported:

* ``sink``: string

    Sink name; default is ``"@DEFAULT_SINK@"``.

* ``make_self_pipe``: boolean

    If ``true``, the ``wake_up()`` function (see `Functions`_) will be available.

``cb`` argument
===============

* If the ``make_self_pipe`` option is set to ``true``, and the callback is invoked because of the
  call to ``wake_up()``, then the argument is ``nil``;

* otherwise, the argument is a table with the following entries:

  - ``cur``: integer

      Current volume level.

  - ``norm``: integer

      "Normal" (corresponding to 100%) volume level.

  - ``mute``: boolean

      Whether the sink is mute.

  - ``index``: integer

      Index of the default sink.

  - ``name``: string

      Name of the default sink.

  - ``desc``: string

      Description of the default sink.

  - ``port_name``: string

      Name of the default sink's active port.

  - ``port_desc``: string

      Description of the default sink's active port.

  - ``port_type``: string

      Type of the default sink's active port.
      Possible values are:

      - ``"unknown"``

      - ``"aux"``

      - ``"speaker"``

      - ``"headphones"``

      - ``"line"``

      - ``"mic"``

      - ``"headset"``

      - ``"handset"``

      - ``"earpiece"``

      - ``"spdif"``

      - ``"hdmi"``

      - ``"tv"``

      - ``"radio"``

      - ``"video"``

      - ``"usb"``

      - ``"bluetooth"``

      - ``"portable"``

      - ``"handsfree"``

      - ``"car"``

      - ``"hifi"``

      - ``"phone"``

      - ``"network"``

      - ``"analog"``

Functions
=========
The following functions are provided:

* ``wake_up()``

    Forces a call to ``cb`` (with ``nil`` argument).

    Only available if the ``make_self_pipe`` option was set to ``true``; otherwise, it throws an
    error.
