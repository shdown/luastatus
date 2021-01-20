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

* ``how``: string

    This option controls which method of obtaining the list of group names is used.
    Currently, there are two methods.

    The first one, codenamed "*wrongly*", is the default one; it consists of
    querying and parsing the ``_XKB_RULES_NAMES`` property of the root window.
    This method is known to be wrong, and on current Debian Sid it does not work as expected:
    on a setup with two keyboard layouts, English and Russian, it reports only the English one.
    Specify ``how="wrongly"`` to use this method.

    The second one, codenamed "*somehow*", consists of calling
    ``XkbGetNames(..., XkbSymbolsNameMask, ...)`` in order to obtain a string called "symbols",
    which looks like this::

        pc+us+ru(winkeys):2+inet(evdev)+group(rctrl_toggle)+level3(ralt_switch)+capslock(ctrl_modifier)+typ

    Note that this string has been truncated to exactly 99 characters (the end should have been
    ``typo`` and then probably something else).
    It was not me who truncated it, but rather the guts of X11. And I have no idea why.
    The fact that this string can be truncated certainly does not contribute to the reliability of
    this method.
    We obtain the list of group names, then, by splitting the "symbols" by plus signs, and then
    filtering out known "bad" symbols (that do not indicate a keyboard layout).
    This is quite unreliable, but somehow works.
    Specify ``how="somehow"`` to use this method.

* ``somehow_bad``: string

    Comma-separated list of bad symbols for the "somehow" method (note that normally it should
    not include spaces). Set to empty string to express an empty list.
    The default value is ``group,inet,pc``.

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
