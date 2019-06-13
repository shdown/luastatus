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
This plugin monitors the volume and mute status of the default PulseAudio sink.

Options
=======
The following options are supported:

* ``sink``: string

    Sink name; default is ``"@DEFAULT_SINK@"``.

* ``timeout``: number

    If specified and not negative, this plugin will call ``cb`` with ``nil`` argument whenever the
    channel does not change its state in ``timeout`` seconds since the previous call to ``cb``.

* ``make_self_pipe``: boolean

    If ``true``, the ``wake_up()`` function (see `Functions`_) will be available.

``cb`` argument
===============
On timeout, ``nil`` (if the ``timeout`` option has been specified).

Otherwise, the argument is a table with the following entries:

* ``cur``: integer

    Current volume level.

* ``norm``: integer

    "Normal" (corresponding to 100%) volume level.

* ``mute``: boolean

    Whether the sink is mute.

Functions
=========
The following functions are provided:

* ``wake_up()``

    Forces a call to ``cb``.

    Only available if the ``make_self_pipe`` option was set to ``true``; otherwise, it throws an
    error.
