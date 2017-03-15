luastatus is a universal status bar content generator. It allows you to
configure the way the data from plugins is processed and shown with Lua.

Its main feature is that the content can be updated immediately as some event
occurs, be it a change of keyboard layout, active window title, volume or a song
in your favorite player (if there is a plugin for it, of course) — a thing
rather uncommon for tiling window managers.

Its motto is:

> No more heavy-forking, second-lagging shell-script status bar generators!

Screenshot
===

![...](https://cloud.githubusercontent.com/assets/22565120/19625163/fecb2310-9921-11e6-90f3-17d291278531.gif)

Above is i3bar with luastatus with time, battery, volume and keyboard layout
widgets. Widgets and i3bar configuration are
[here](https://github.com/shdown/luastatus/tree/master/contrib/widget-examples).

Widgets
===
A widget is a Lua program with a table named `widget` defined (except for
`luastatus`, you can freely define and modify any other global variables).

The `widget` table **must** contain the following entries:

  * `plugin`: a string with the name of a *plugin* (see below) you want to
  receive data from. If it contains a slash, it is treated as a path to a shared
  library. If it does not, luastatus tries to load `<plugin>.so` from the
  directory configured at the build time (CMake `PLUGINS_DIR` variable, defaults
  to `${CMAKE_INSTALL_FULL_LIBDIR}/luastatus/plugins`).

  * `cb`: a function that converts the data received from a *plugin* to the
  format a *barlib* (see below) understands. It should take exactly one
  argument and return exactly one value.

The `widget` table **may** contain the following entries:

  * `opts`: a table with plugin’s options. If undefined, an empty table will be
  substituted.

  * `event`: a function or a string.
   - If is a function, it is called by the *barlib* when some event with the
     widget occurs (typically a click). It should take exactly one argument and
     not return anything;
   - if is a string, it is compiled as a function in a *separate state*, and
     when some event with the widget occurs, the compiled function is called in
     that state (not in the widget’s state, in which `cb` gets called). This may
     be useful for widgets that want not to receive data from plugin, but to
     generate it themselves (possibly using some external modules). Such a
     widget may want to specify `plugin = 'timer', opts = {period = 0}` and
     block in `cb` until it wants to update. The problem is that in this case,
     widget’s Lua mutex is almost always being acquired by `cb`, and there is
     very little change for `event` to get called. A separate-state `event`
     function solves that.

Plugins
===
A plugin is a thing that knows when to call the `cb` function and what to pass
to.

Plugins are shared libraries. For how to write them, see
`DOCS/WRITING_BARLIB_OR_PLUGIN.md`.

Barlibs
===
A barlib (**bar** **lib**rary) is a thing that knows:

  * what to do with values the `cb` function returns;

  * when to call the `event` function and what to pass to;

  * how to indicate an error, should one happen.

Barlibs are shared libraries, too. For how to write them, see
`DOCS/WRITING_BARLIB_OR_PLUGIN.md`.

Barlibs are capable of taking options.

The event loop
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
luastatus provides the `luastatus` module, which contains the following
functions:

* `luastatus.rc(args)`

  Spawns a process, waits for its termination, and returns its exit status if it
  exited normally, 128+**n** if it was killed by a signal with number **n**, or
  127 if there was an error spawning the process.

* `luastatus.dollar(args)`

  Spawns a process with stdout redirected to an internal buffer, waits for its
  termination, and returns output and exit status (rules for `luastatus.rc`
  apply). Note that it drops all trailing newlines, as all shells do, so that
  `luastatus.dollar{"echo", "hello"} == "hello"` (not `"hello\n"`).

  It is named so because of its similarity to the `$(...)` shell construct.

* `luastatus.spawn(args)`

  Spawns a process, does **not** wait for its termination, and does not return
  anything.

Plugins and barlibs can also register Lua functions. They appear in
`luastatus.plugin` and `luastatus.barlib` submodules correspondingly.

Lua limitations
===
luastatus prohibits `os.execute`, `os.setlocale`, and `os.exit` as they are not
thread-safe. It’s pretty easy to implement `os.execute` in terms of
`luastatus.rc`, and the other two just shouldn’t be used in widgets.

Supported Lua versions
===
* 5.1
* LuaJIT, which is currently 5.1-compatible with “some language and library extensions from Lua 5.2”
* 5.2
* 5.3

Usage
===
Pass a barlib with `-b`, then (optionally) its options with `-B`, then widget
files.

If `-b` argument contains a slash, it is treated as a path to a shared library.
If it does not, luastatus tries to load `<argument>.so` from the directory
configured at the build time (CMake `BARLIBS_DIR` variable, defaults to
`${CMAKE_INSTALL_FULL_LIBDIR}/luastatus/barlibs`).

Example:

    luastatus -b dwm -B display=:0 -B separator=' ' widget1.lua widget2.lua

Note that some barlibs can provide their own wrappers for luastatus.

Installation
===
`cmake . && make && sudo make install`

You can specify a Lua library to build with: `cmake -DWITH_LUA_LIBRARY=luajit .`.

You can disable building certain barlibs and plugins, e.g. `cmake -DBUILD_PLUGIN_XTITLE=OFF .`.

Reporting bugs, requesting features, suggesting patches
===
Feel free to open an issue or a pull request.
