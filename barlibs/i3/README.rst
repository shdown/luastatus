.. :X-man-page-only: luastatus-barlib-i3
.. :X-man-page-only: #####################
.. :X-man-page-only:
.. :X-man-page-only: ##########################
.. :X-man-page-only: i3 barlib for luastatus
.. :X-man-page-only: ##########################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This barlib talks with ``i3bar``.

To use this barlib, you need to specify ``luastatus-i3-wrapper`` with appropriate arguments as the
``status_command`` parameter of a bar in the i3 configuration file. For example::

    bar {
        status_command cd ~/.config/luastatus && exec luastatus-i3-wrapper -B no_separators time-battery-combined.lua alsa.lua xkb.lua

Redirections and ``luastatus-i3-wrapper``
=========================================
``i3bar`` requires all the data to be written to stdout and read from stdin.

This makes it very easy
to mess things up: Lua's ``print()`` prints to stdout, processes spawned by widgets/plugins inherit
our stdin and stdout, etc.

That's why this barlib requires that stdin and stdout file descriptors are manually redirected.

A shell wrapper, ``luastatus-i3-wrapper``, is shipped with it; it does all the redirections and
executes ``luastatus`` with ``-b i3``, all the required ``-B`` options, and additional arguments
passed by you.

``cb`` return value
===================
Either of:

* an empty table or ``nil``

    Hide the widget.

* a table with strings keys

    Is interpreted as a single segment (or a "block"). The keys *should not* include ``name``, as it
    is set automatically to be able to tell which widget was clicked.

    For more information, see http://i3wm.org/docs/i3bar-protocol.html#_blocks_in_detail.

* an array (table with numeric keys)

    Is interpreted as an array of segments. To be able to tell which one was clicked, set the
    ``instance`` field of a segment.

    ``nil`` elements are ignored.

``event`` argument
==================
A table with all click properties ``i3bar`` provides.

For more information, see http://i3wm.org/docs/i3bar-protocol.html#_click_events.

Functions
=========
The following functions are provided:

* ``escaped_str = luastatus.barlib.pango_escape(str)``

    Escapes text for the Pango markup.

Example
=======
An example that uses all possible features::

    function get_user_name()
        local f = io.popen('whoami', 'r')
        local r = f:read('*line')
        f:close()
        return r
    end

    i = 0
    widget = {
        plugin = 'timer',
        opts = {period = 2},
        cb = function(t)
            i = i + 1
            if i == 1 then
                return {} -- hides widget; alternatively, you can return nil.
            elseif i == 2 then
                -- no markup unless you specify markup='pango'
                return {full_text = '<Hello!>', color = '#aaaa00'}
            elseif i == 3 then
                -- see https://developer.gnome.org/pango/stable/PangoMarkupFormat.html
                return {full_text = 'Hello, <span color="#aaaa00">'
                                    .. luastatus.barlib.pango_escape(get_user_name())
                                    .. '</span>!',
                        markup = 'pango'}
            elseif i == 4 then
                i = 0
                return {
                    {full_text = 'Now,', instance = 'now-segment'},
                    nil, -- nils are ignored so that you can do
                         --     return {get_time(), get_battery(), get_smth_else()}
                         -- where, for example, get_battery() returns nil if the battery is full.
                    {full_text = 'click me!', instance = 'click-me-segment'},
                }
            end
        end,
        event = function(t)
            local r = {'properties:'}
            for k, v in pairs(t) do
                table.insert(r, k .. '=' .. tostring(v))
            end
            assert(os.execute(string.format(
               "notify-send 'Click event' '%s'", table.concat(r, ' '))))
        end,
    }

Options
=======
The following options are supported:

* ``in_fd=<fd>``

    File descriptor to read ``i3bar`` input from. Usually set by the wrapper.

* ``out_fd=<fd>``

    File descriptor to write to. Usually set by the wrapper.

* ``no_click_events``

    Tell ``i3bar`` we don't want to receive click events. This changes ``i3bar`` behaviour in that
    it will interpret "clicks" on segments as if an empty space on the bar was clicked,
    particularly, will switch workspaces if you scroll on a segment.

* ``no_separators``

    Append ``"separator": false`` to a segment, unless it has a ``separator`` key. Also appends it
    to an ``(Error)`` segment.

* ``allow_stopping``

    Allow i3bar to send luastatus ``SIGSTOP`` when it thinks it becomes invisible, and ``SIGCONT``
    when it thinks it becomes visible. Quite a questionable feature.
