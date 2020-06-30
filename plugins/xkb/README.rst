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

* ``requery``: boolean

    True if either this is the first call, or this call is due to a change in keyboard geometry, the
    *list* of layouts, or similar event that requires re-query of the list of group names.
    Otherwise, ``requery`` is nil. This is useful for widgets that fetch the list of group names via
    some external mechanism (e.g. by parsing the output of a command): they need to re-query the
    list if ``requery`` is true.

* ``led_state``: number

    Bit mask representing the current state of LED indicators.

    On virtually all setups,
    bit ``(1 << 0) = 1`` is "Caps Lock",
    bit ``(1 << 1) = 2`` is "Num Lock",
    bit ``(1 << 2) = 4`` is "Scroll Lock".

    To list all indicators your X server knows of, run ``xset q``.
    If you look at the output, you will note there are a lot of weird things that are, in some
    reason, also considered indicators; for example, "Group 2" (that is, alternative keyboard
    layout) corresponds to ``(1 << 12) = 4096`` on my setup. So you should not assume that, just
    because your physical keyboard has no indicators for other things (or no indicators at all),
    the mask will never contain anything other than "Caps Lock", "Num Lock" and "Scroll Lock".

    Note that bitwise operations were only introduced in Lua 5.3.
    The "lowest common denominator" (working on all Lua versions) check if a bit is set is
    the following::

        function is_set(mask, bit)
            return mask % (2 * bit) >= bit
        end

    Use it as follows::

        cb = function(t)
            -- ...do something...

            if is_set(t.led_state, 1) then
                -- "Caps Lock" is ON
                -- ...do something...
            end

            if is_set(t.led_state, 2) then
                -- "Num Lock" is ON
                -- ...do something...
            end

            if is_set(t.led_state, 4) then
                -- "Scroll Lock" is ON
                -- ...do something...
            end

            -- ...do something...
        end,
