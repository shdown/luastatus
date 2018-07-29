**luastatus** is a universal status bar content generator. It allows you to
configure the way the data from event sources is processed and shown, with Lua.

Its main feature is that the content can be updated immediately as some event
occurs, be it a change of keyboard layout, active window title, volume or a song
in your favorite music player (provided that there is a plugin for it) — a thing
rather uncommon for tiling window managers.

Its motto is:

> No more heavy-forking, second-lagging shell-script status bar generators!

Screenshot
===

![...](https://user-images.githubusercontent.com/5462697/39099519-092459aa-4685-11e8-94fe-0ac1cf706d82.gif)

Above is i3bar with luastatus with Bitcoin price, time, volume, and keyboard
layout widgets.

Key concepts
===

![Explanation](https://user-images.githubusercontent.com/5462697/42400208-5b54f5f2-8179-11e8-9836-70d4e46d5c13.png)

Widgets
---
A widget is a Lua program with a table named `widget` defined (except for
`luastatus`, you can freely define and modify any other global variables).

The `widget` table **must** contain the following entries:

  * `plugin`: a string with the name of a *plugin* (see below) you want to
  receive data from. If it contains a slash, it is treated as a path to a shared
  library. If it does not, luastatus tries to load `<plugin>.so` from the
  directory configured at the build time (CMake `PLUGINS_DIR` variable, defaults
  to `${CMAKE_INSTALL_FULL_LIBDIR}/luastatus/plugins`).

  * `cb`: a function that converts the data received from a *plugin* to the
  format a *barlib* (see below) understands. It should take one argument (though
  in Lua you can omit it if you don’t care about its value) and return one value
  (`nil` is substituted if no values are returned).

The `widget` table **may** contain the following entries:

  * `opts`: a table with plugin’s options. If undefined, an empty table will be
  substituted.

  * `event`: a function or a string.
    - If is a function, it will be called by the *barlib* whenever some event
      with the widget occurs (typically a click). It should take one argument
      and not return anything;
    - if is a string, it is compiled as a function in a *separate state*. See
     `DOCS/design/separate_state.md` for overview.

Plugins
---
A plugin is a thing that knows when to call the `cb` function and what
to pass to.

Plugins are shared libraries.

These plugins are also referred to as *proper plugins*, as opposed to
*dervied plugins*.

Following is an example of widget for the `i3` barlib that uses a (proper)
plugin.

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

Derived plugins
---
Starting with version 0.3.0, luastatus features *derived plugins*, which are
wrappers around proper plugins (or even other derived plugins) written in Lua.

Derived plugins are loaded by calling `luastatus.require_plugin(name)`.

Following is an example of widget for the `i3` barlib that uses a derived
plugin.

```lua
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
    sleep_on_error = function() os.execute('sleep 60') end,
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

Barlibs
---
A barlib (**bar** **lib**rary) is a thing that knows:

  * what to do with values the `cb` function returns;

  * when to call the `event` function and what to pass to;

  * how to indicate an error, should one happen.

Barlibs are shared libraries, too.

Barlibs are capable of taking options.

Installation
===
`cmake . && make && sudo make install`

You can specify a Lua library to build with: `cmake -DWITH_LUA_LIBRARY=luajit .`

You can disable building certain barlibs and plugins, e.g. `cmake -DBUILD_PLUGIN_XTITLE=OFF .`

Getting started
===
First, find your barlib’s subdirectory in the `barlibs/` directory. Then read
its `README.md` file for detailed instructions and documentation.

Similary, for plugins’ documentation, see `README.md` files in the
subdirectories of `plugins/`.

You will find widget examples in `contrib/widget-examples`.

Using luastatus binary
===
Note that some barlibs can provide their own wrappers for luastatus; that’s why
you should consult your barlib’s `README.md` first.

Pass a barlib with `-b`, then (optionally) its options with `-B`, then widget
files.

If `-b` argument contains a slash, it is treated as a path to a shared library.
If it does not, luastatus tries to load `<argument>.so` from the directory
configured at the build time (CMake `BARLIBS_DIR` variable, defaults to
`${CMAKE_INSTALL_FULL_LIBDIR}/luastatus/barlibs`).

Example:

    luastatus -b dwm -B display=:0 -B separator=' ' widget1.lua widget2.lua

How it works
===
Each widget runs in its own thread and has its own Lua interpreter instance.

While Lua does support multiple interpreters running in separate threads, it
does not support multithreading within one interpreter, which means `cb()` and
`event()` of the same widget never overlap (a widget-local mutex is acquired
before calling any of these functions, and is released afterwards).

Also, due to luastatus’ architecture, no two `event()` functions, even from
different widgets, can overlap. (Note that `cb()` functions from different
widgets can overlap.)

Lua libraries
===

The `luastatus` module
---
luastatus provides the `luastatus` module, which contains the following
functions:
  - `luastatus.require_plugin(name)` is like the `require` function, except that
  it loads a file named `<name>.lua` from luastatus’ plugins directory.
  - `luastatus.map_get_handle(key)` returns a handle object to the *map entry*
  with the key `key`. See `DOCS/design/map_get.md` for overview; in short, this
  is a mechanism allowing plugins to synchronize thread-unsafe operations.
  Unless you’re writing a derived plugin *and* using thread-unsafe stuff, you
  don’t need it.

Plugins’ and barlib’s Lua functions
---
Plugins and barlibs can register Lua functions. They appear in
`luastatus.plugin` and `luastatus.barlib` submodules, correspondingly.

Limitations
---
In luastatus, `os.setlocale` always fails as it is inherently not thread-safe.

Supported Lua versions
===
* 5.1
* LuaJIT, which is currently 5.1-compatible with “some language and library extensions from Lua 5.2”
* 5.2
* 5.3
* 5.4 (`work1` pre-release version)

Reporting bugs, requesting features, suggesting patches
===
Feel free to open an issue or a pull request.

Migrating from older versions
===
See `DOCS/MIGRATION_GUIDE.md`.
