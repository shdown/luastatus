If `widget.cb` is a string, it gets compiled  as a function in a
*separate state*.

Whenever some event with the widget occurs, the compiled function will be called
in that state (not in the widget’s state, in which `cb` gets called).

This may be useful for widgets that want not to receive data from plugin, but to
generate it themselves (possibly using some external modules). Such a widget may
want to specify
```lua
widget = {
    plugin = 'timer',
    opts = {period = 0},
    …
```
and block in `cb` until it wants to update. The problem is that in this case,
widget’s Lua mutex is almost always being acquired by `cb`, and there is very
little chance for `event` to get called. A separate-state `event` function
solves that.
