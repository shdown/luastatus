.. :X-man-page-only: luastatus-plugin-xkb
.. :X-man-page-only: ####################
.. :X-man-page-only:
.. :X-man-page-only: ######################################
.. :X-man-page-only: X keyboard layout plugin for luastatus
.. :X-man-page-only: ######################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors current keyboard layout, and, optionally, the state of LED indicators (such as
"Caps Lock" and "Num Lock").

Options
=======
The following options are supported:

* ``display``: string

    Display to connect to. Default is to use ``DISPLAY`` environment variable.

* ``device_id``: number

    Keyboard device ID (as shown by ``xinput(1)``). Default is ``XkbUseCoreKbd``.

* ``led``: boolean

    Also report (and subscribe to changes of) the state of LED indicators, such as "Caps Lock" and
    "Num Lock".

``cb`` argument
===============
A table with the following entries:

* ``name``: string

    Group name (if number of group names reported is sufficient).

* ``id``: number

    Group ID (0, 1, 2, or 3).

* ``led_state``: number

    Bit mask representing the current state of LED indicators.

    On virtually all setups,
    bit `(1 << 0) = 1` is "Caps Lock",
    bit `(1 << 1) = 2` is "Num Lock",
    bit `(1 << 2) = 4` is "Scroll Lock".

    To list all XKB indicators your X server knows of, see the output of ``xset q``.

    Note that bitwise operations were only introduced in Lua 5.3.
    The "lowest common denominator" (working on all Lua versions) check if a bit is set is
    the following:
    ``function is_set(mask, bit) return mask % (2 * bit) >= bit end``
