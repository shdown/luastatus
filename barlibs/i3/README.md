This barlib talks with `i3bar`.

Redirections and `luastatus-i3-wrapper`
===
`i3bar` requires all the data to be written to stdout and read from stdin. 
This makes it very easy to mess things up: Lua’s `print()` prints to stdout, processes spawned by widgets/plugins inherit our stdin and stdout, etc.

That’s why this barlib requires that stdin and stdout file descriptors are manually redirected. A shell wrapper, `luastatus-i3-wrapper`, is shipped with it; it does all the redirections and executes `luastatus` (or whatever is in `LUASTATUS` environment variable) with `-b i3` (or whatever is in `LUASTATUS_I3_BARLIB` environment variable), all the required `-B` options, and additional arguments passed by you.

`cb` return value
===
* An empty table or `nil` hides a widget.
* A table with string keys is interpreted as a single segment. The keys **should not** include `name`, as it is set automatically to be able to tell which widget was clicked.
* An array (that is, a table with numeric keys) is interpreted as an array of segments. To be able to tell which one was clicked, set the `instance` field.

See http://i3wm.org/docs/i3bar-protocol.html#_blocks_in_detail.

`event` argument
===
A table with all click properties `i3bar` provides, with `name` excluded.

See http://i3wm.org/docs/i3bar-protocol.html#_click_events.

Functions
===
* `escaped_str = luastatus.plugin.pango_escape(str)`

  Escapes text for Pango markup.

Example
===
````lua
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
            return {full_text = 'Hello, <span color="#aaaa00">' .. luastatus.barlib.pango_escape(luastatus.dollar{'whoami'}) .. '</span>!',
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
        local r = 'properties:'
        for k, v in pairs(t) do
            table.insert(r, k .. '=' .. v)
        end
        assert(os.execute('notify-send "Click event!" \'' .. table.concat(r, ' ') .. '\''))
    end,
}
````

Options
===
* `in_fd=<fd>`

  File descriptor to read `i3bar` input from. Usually set by the wrapper.

* `out_fd=<fd>`

  File descriptor to write to. Usually set by the wrapper.

* `no_click_events`

  Tell `i3bar` we don’t want to receive click events. This changes `i3bar`
  behaviour in that it will interpret “clicks” on segments as if an empty space
  on the bar was clicked, particulary, will switch workspaces if you scroll
  on a segment.

* `no_separators`

  Append `"separator": false` to a segment, unless it has a `separator` key.
  Also appends it to an `(Error)` segment (convenient, isn’t it?).

* `allow_stopping`

  Allow i3bar to send luastatus `SIGSTOP` when it thinks it becomes invisible,
  and `SIGCONT` when it thinks it becomes visible.
  Quite a questionable feature.
