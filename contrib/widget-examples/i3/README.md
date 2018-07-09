Excerpt from `~/.config/i3/config`:

````
bar {
    status_command cd ~/.config/luastatus && exec luastatus-i3-wrapper -B no_separators time-battery-combined.lua alsa.lua xkb.lua
````
