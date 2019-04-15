.. :X-man-page-only: luastatus-plugin-alsa
.. :X-man-page-only: #####################
.. :X-man-page-only:
.. :X-man-page-only: #########################
.. :X-man-page-only: ALSA plugin for luastatus
.. :X-man-page-only: #########################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors volume and mute state of an ALSA channel.

Options
========
The following options are supported:

* ``card``: string

  Card name, defaults to ``"default"``.

* ``channel``: string

  Channel name, defaults to ``"Master"``.

* ``in_db``: boolean

  Whether or not to report normalized volume (in dBs).

* ``capture``: boolean

  Whether or not this is a capture stream, as opposed to a playback one. Defaults to false.

* ``make_self_pipe``: boolean

  If true, the ``wake_up()`` (see the `Functions`_ section) function will be available. Defaults to
  false.

``cb`` argument
===============
A table with the following entries:

* ``mute``: boolean

    Whether or not the playback switch is turned off.

    (Only provided if the channel has the playback switch capability.)

* ``vol``: table with the following entries:

  * ``cur``, ``min``, ``max``: numbers

      Current, minimal and maximal volume levels, correspondingly.

      (Only provided if the channel has the volume control capability.)

Functions
=========
The following functions are provided:

* ``wake_up()``

    Forces a call to ``cb``.

    Only available if the ``make_self_pipe`` option was set to ``true``; otherwise, it throws an
    error.
