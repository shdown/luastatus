.. :X-man-page-only: luastatus-plugin-mpris
.. :X-man-page-only: ######################
.. :X-man-page-only:
.. :X-man-page-only: #######################################
.. :X-man-page-only: MPRIS media player plugin for luastatus
.. :X-man-page-only: #######################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin interfaces with MPRIS-compatible media players
via D-Bus to monitor playback status and track metadata.

Functions
=========
The following functions are provided:

* ``widget(tbl)``

  Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with the following fields:

  **(required)**

  - ``player``: a string

    The name of the MPRIS player (e.g., ``"clementine"``).
    It will be automatically prefixed with ``org.mpris.MediaPlayer2.`` when communicating over D-Bus.
    You can list active MPRIS players on your system by running the following command::

        dbus-send \
            --session --dest=org.freedesktop.DBus --type=method_call --print-reply \
            /org/freedesktop/DBus org.freedesktop.DBus.ListNames \
            | grep -F 'org.mpris.MediaPlayer2'


  - ``cb``: a function

    The callback function that will be called with a table containing the current MPRIS player
    properties (such as ``Metadata``, ``PlaybackStatus``, ``Volume``, etc.).
    The table will be empty if the data cannot be retrieved or the player is unavailable.

  **(optional)**

  - ``forced_refresh_interval``: a number

    The interval in seconds for manually querying the player properties.
    Defaults to 30.
    This acts as a safety fallback if D-Bus signals are missed or invalidated.

  - ``event``
    The ``event`` entry of the resulting table (see ``luastatus`` documentation for the description of ``widget.event`` field).
