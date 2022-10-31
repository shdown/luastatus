[![CircleCI build status](https://circleci.com/gh/shdown/luastatus.svg?style=shield)](https://circleci.com/gh/shdown/luastatus)
[![Gitter](https://badges.gitter.im/luastatus/community.svg)](https://gitter.im/luastatus/community)

**luastatus** is a universal status bar content generator. It allows you to configure the way the
data from event sources is processed and shown, with Lua.

Its main feature is that the content can be updated immediately as some event occurs, be it a change
of keyboard layout, active window title, volume or a song in your favorite music player (provided
that there is a plugin for it) — a thing rather uncommon for tiling window managers.

Its motto is:

> No more heavy-forking, second-lagging shell-script status bar generators!

Screenshot
===

![Screenshot](https://user-images.githubusercontent.com/5462697/39099519-092459aa-4685-11e8-94fe-0ac1cf706d82.gif)

Above is i3bar with luastatus with Bitcoin price, time, volume, and keyboard layout widgets.

Key concepts
===

![Explanation](https://user-images.githubusercontent.com/5462697/42400208-5b54f5f2-8179-11e8-9836-70d4e46d5c13.png)

In short:
  * plugin is a thing that decides when to call the callback function `widget.cb` and what to pass to it;
  * barlib (**bar** **lib**rary) is a thing that decides what to with values that `widget.cb` function returns;
  * there are also *derived plugins*, which are plugins written in Lua that use regular plugins.

Examples
===
ALSA volume widget:

```lua
widget = {
    plugin = 'alsa',
    opts = {
        channel = 'PCM'
    },
    cb = function(t)
        if t.mute then
            return {full_text = '[mute]', color = '#e03838'}
        else
            local percent = (t.vol.cur - t.vol.min)
                          / (t.vol.max - t.vol.min)
                          * 100
            return {full_text = string.format('[%3d%%]', math.floor(0.5 + percent)),
                    color = '#718ba6'}
        end
    end,
    event = function(t)
        if t.button == 1 then     -- left mouse button
            os.execute('urxvt -e alsamixer &')
        end
    end
}
```

GMail widget (uses the derived plugin `imap`):

```lua
--[[
-- Expects 'credentials.lua' to be present in the current directory; it may contain, e.g.,
--     return {
--         gmail = {
--             login = 'john.smith',
--             password = 'qwerty'
--         }
--     }
--]]
credentials = require 'credentials'
widget = luastatus.require_plugin('imap').widget{
    host = 'imap.gmail.com',
    port = 993,
    mailbox = 'Inbox',
    use_ssl = true,
    timeout = 2 * 60,
    handshake_timeout = 10,
    login = credentials.gmail.login,
    password = credentials.gmail.password,
    error_sleep_period = 60,
    cb = function(unseen)
        if unseen == nil then
            return nil
        elseif unseen == 0 then
            return {full_text = '[-]', color = '#595959'}
        else
            return {full_text = string.format('[%d unseen]', unseen)}
        end
    end,
    event = [[                    -- separate-state event function
        local t = ...             -- obtain argument of this implicit function
        if t.button == 1 then     -- left mouse button
            os.execute('xdg-open https://gmail.com &')
        end
    ]]
}
```

See more examples [here](https://github.com/shdown/luastatus/tree/master/examples).

Installation
===
`cmake . && make && sudo make install`

You can specify a Lua library to build with: `cmake -DWITH_LUA_LIBRARY=luajit .`

You can disable building certain barlibs and plugins, e.g. `cmake -DBUILD_PLUGIN_XTITLE=OFF .`

You can disable building man pages: `cmake -DBUILD_DOCS=OFF .`

ArchLinux
---
ArchLinux users can use one of the following AUR packages:

* [luastatus](https://aur.archlinux.org/packages/luastatus)
* [luastatus-luajit](https://aur.archlinux.org/packages/luastatus-luajit)
* [luastatus-git](https://aur.archlinux.org/packages/luastatus-git)
* [luastatus-luajit-git](https://aur.archlinux.org/packages/luastatus-luajit-git)

There is also the [luastatus-meta](https://aur.archlinux.org/packages/luastatus-meta)
meta package which installs the dependencies of all of luastatus's plugins.

Getting started
===
It is recommended to first have a look at the
[luastatus' man page](https://github.com/shdown/luastatus/blob/master/luastatus/README.rst).

Then, read the barlib's and plugins' documentation, either via directly viewing
`barlibs/<name>/README.rst` and `plugins/<name>/README.rst` files, or via installing the man pages
and reading `luastatus-barlib-<name>(7)` and `luastatus-plugin-<name>(7)`.

Barlib-specific notes on usage follow.

i3
---
`luastatus-i3-wrapper` should be specified as the i3bar's status command in the i3 config, e.g.:
```
bar {
    status_command cd ~/.config/luastatus && exec luastatus-i3-wrapper -B no_separators time-battery-combined.lua alsa.lua xkb.lua
```

See also [README for i3](https://github.com/shdown/luastatus/blob/master/barlibs/i3/README.rst) and
[examples for i3](https://github.com/shdown/luastatus/tree/master/examples/i3).

dwm
---
luastatus should simply be launched with `-b dwm`, e.g.:
```
luastatus -b dwm -B separator=' • ' alsa.lua time-battery-combined.lua
```

See also [README for dwm](https://github.com/shdown/luastatus/blob/master/barlibs/dwm/README.rst)
and [examples for dwm](https://github.com/shdown/luastatus/tree/master/examples/dwm).

lemonbar
--------
`lemonbar` should be launched with `luastatus-lemonbar-launcher`, e.g.:
```
luastatus-lemonbar-launcher -p -B#111111 -p -f'Droid Sans Mono for Powerline:pixelsize=12:weight=Bold' -- -Bseparator=' ' alsa.lua time-date.lua
```

See also
[README for lemonbar](https://github.com/shdown/luastatus/blob/master/barlibs/lemonbar/README.rst)
and [examples for lemonbar](https://github.com/shdown/luastatus/tree/master/examples/lemonbar).

stdout
------
luastatus should be launched with `luastatus-stdout-wrapper`; or write your own wrapper, see e.g.
the [wrapper for launching dvtm with luastatus](https://github.com/shdown/luastatus/blob/master/barlibs/stdout/luastatus-dvtm).

See also
[README for stdout](https://github.com/shdown/luastatus/blob/master/barlibs/stdout/README.rst) and
and [examples for stdout](https://github.com/shdown/luastatus/tree/master/examples/stdout).

Supported Lua versions
===
* 5.1
* LuaJIT, which is currently 5.1-compatible with "some language and library extensions from Lua 5.2"
* 5.2
* 5.3
* 5.4

Reporting bugs, requesting features, suggesting patches
===
Feel free to open an issue or a pull request. You can also chat on [our Gitter chat room](https://gitter.im/luastatus/community).

Migrating from older versions
===
See the [Migration Guide](https://github.com/shdown/luastatus/blob/master/DOCS/MIGRATION_GUIDE.md).

Acknowledgements
===
* I would like to thank [wm4](https://github.com/wm4) for developing [mpv](https://mpv.io), which,
  also being a “platform” for running Lua scripts, served as an inspiration for this project.

Donate
===

You can [donate to our collective on Open Collective](https://opencollective.com/luastatus/donate).
