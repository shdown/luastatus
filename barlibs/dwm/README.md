This barlib sets the name of the root window.

It joins all non-empty strings returned by widgets by a separator, which defaults to ` | `.

It does not provide functions and does not support events.

`cb` return value
===
A string or nil. Nil is equivalent to an empty string.

Example
===
````lua
widget = {
    plugin = 'timer',
    cb = function(t)
        return os.date('%H:%M')
    end,
}
````

Options
===
* `display=<name>`

  Set the name of a display to connect to. Default is to use `DISPLAY` environment variable.

* `separator=<string>`

  Set the separator.
