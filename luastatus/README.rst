luastatus
#########

######################################
universal status bar content generator
######################################

:Copyright: LGPLv3
:Manual section: 1

SYNOPSIS
========
**luastatus** **-b** *barlib* [**-B** *barlib_option*]... [**-l** *loglevel*] [**-e**] *widget_file*...

**luastatus** **-v**

DESCRIPTION
===========
**luastatus** is a universal status bar content generator. It allows you to configure the way the
data from event sources is processed and shown, with Lua.

OPTIONS
=======
-b barlib
   Specify a barlib to use. If *barlib* contains a slash, it is treated as a path to a shared
   library. If it does not, the program tries to load barlib-*barlib*.so from the directory
   configured at the build time (CMake ``BARLIBS_DIR`` variable, defaults to
   ``${CMAKE_INSTALL_FULL_LIBDIR}/luastatus/barlibs``).

-B barlib_option
   Pass an option to *barlib*. May be specified multiple times.

-l loglevel
   Specify a log level. *loglevel* is one of: *fatal error warning info verbose debug trace*.

   Default is *info*.

-e
   Do not hang, but exit normally when *barlib*'s event watcher and all plugins' ``run()`` have
   returned. Default behaviour is to hang, because there are status bars that require their
   generator process not to terminate (namely i3bar).

   Useful for testing.

-v
   Show version and exit.

WIDGETS
=======
A widget is a Lua program with a table named ``widget`` defined. The ``widget`` table **must**
contain the following entries:

* ``plugin``: string

  Name or the path to a *plugin* (see `PLUGINS`_) the widget wants to receive data from. If it
  contains a slash, it is treated as a path to a shared library. If it does not, luastatus tries
  to load ``plugin-<plugin>.so`` from the directory configured at the build time (CMake
  ``PLUGINS_DIR`` variable, defaults to ``${CMAKE_INSTALL_FULL_LIBDIR}/luastatus/plugins``).

* ``cb``: function

  The callback that converts the data received from a *plugin* to the format a *barlib* (see
  `BARLIBS`_) understands. It should take one argument and return one value. (The returned values
  will be coerced to exactly one value, so not returning anything is the same as returning
  ``nil``.)

The ``widget`` table **may** contain the following entries:

* ``opts``: table

  Table with plugin's options. If undefined, an empty table will be substituted.

* ``event``: function or string

  - If is a function, it will be called by the *barlib* whenever some event with the widget occurs
    (typically a click). It should take one argument and not return anything.

  - If is a string, it is compiled as a function in a *separate state* (see `SEPARATE STATE`_).

PLUGINS
=======
Plugins are data providers for widgets.
In other words, a plugin is a thing that knows when to call the ``cb`` function of a widget and
what to pass to it.

Plugins are shared libraries.

These plugins are also referred to as *proper plugins*, as opposed to *dervied plugins*.

DERIVED PLUGINS
===============
Starting with version 0.3.0, luastatus features *derived plugins*, which are wrappers, written in
Lua, around proper plugins (or even other derived plugins).

Derived plugins are loaded by calling ``luastatus.require_plugin(name)`` (see `LUA LIBRARIES`_).

BARLIBS
=======
A barlib (**bar** **lib**\rary) is an adapter that communicates with the status bar.
In other words, a barlib is a thing that knows:

* what to do with values that the ``cb`` function of a widget returns;

* when to call the ``event`` function and what to pass to it;

* how to indicate an error, should one happen.

Just like plugins, barlibs are shared libraries.

Barlibs can take options, which can be passed with ``-B`` command-line options to luastatus.

ARCHITECTURE
============
Each widget runs in its own thread and has its own Lua interpreter instance.

While Lua does support multiple interpreters running in separate threads, it does not support
multithreading within one interpreter, which means ``cb()`` and ``event()`` of the same widget never
overlap (a widget-local mutex is acquired before calling any of these functions, and is released
afterwards).

Also, due to luastatus' architecture, no two ``event()`` functions, even from different widgets, can
overlap. (Note that ``cb()`` functions from different widgets can overlap.)

The takeaway is that the ``event()`` function should not block, or bad things will happen.

LUA LIBRARIES
=============

The ``luastatus`` module
------------------------
luastatus provides the ``luastatus`` module, which currently contains the following functions and
one submodules:

* ``luastatus.require_plugin(name)`` acts like the Lua's built-in ``require`` function, except
  that it loads a file named ``<name>.lua`` from luastatus' derived plugins directory. This
  directory is configured at the build time (CMake ``LUA_PLUGINS_DIR`` variable, defaults to
  ``${CMAKE_INSTALL_FULL_DATAROOTDIR}/luastatus/plugins``).

  The file is read, compiled as a Lua code, and executed, and its return value is returned from
  ``luastatus.require_plugin``.
  If this derived plugin has already been loaded, the cached return value is returned.

* ``luastatus.communicate(action, ...)``: function for communication between the widget's proper
  and separate-state event handler. The communication facility consists of a shared string,
  initially empty. This string can be manipulated with this function as follows:

  - ``luastatus.communicate('read')``: reads and returns the current value of the shared string;

  - ``luastatus.communicate('read_and_clear')``: reads the current value of the shared string,
    resets it to an empty string, and returns the previous value;

  - ``luastatus.communicate('write', new_value)``: sets the current value of the shared string
    to ``new_value``, which must be a string;

  - ``luastatus.communicate('cas', old_value, new_value)``: compare-and-swap operation: reads the
    current value of the shared string; if it is equal to ``old_value``, sets it to ``new_value``
    and returns true; otherwise, returns false. Both ``old_value`` and ``new_value`` must be
    strings.

  Any call to this function is guaranteed to be atomic.

* ``luastatus.libwidechar``: module for width-aware wide char string manipulation. The width of
  a character is the number of cells it occupies in a terminal. This module has the following
  functions:

  - ``luastatus.libwidechar.width(str[, i[, j]])``: returns total width of string ``str``; if
    ``str`` contains an illegal sequence, returns ``nil``. See below for what ``i`` and ``j``
    mean.

  - ``luastatus.libwidechar.truncate_to_width(str, target_width[, i[, j]])``: truncates string ``str``
    to (at most) width ``target_width``. If ``str`` contains an illegal sequence, returns
    ``nil, nil``; otherwise, returns ``result, result_width``, where ``result`` is the truncated
    string, ``result_width`` is the width of ``result``. See below for what ``i`` and ``j`` mean.


  - ``luastatus.libwidechar.make_valid_and_printable(str, replace_bad_with[, i[, j]])``: replaces all
    illegal sequences and non-printable characters with ``replace_bad_with``. Returns the
    result of replacement. Note that is does not check if ``replace_bad_with`` itself contains
    illegal sequences and/or non-printable characters. See below for what ``i`` and ``j`` mean.

  - ``luastatus.libwidechar.is_dummy_implementation()``: returns boolean indicating whether
    the implementation of this module is *dummy*; the implementation is *dummy* if your system
    does not support ``wcwidth()`` function and so the width of any wide character is assumed
    to be 1. Note that missing ``wcwidth()`` is very uncommon.

  All functions above that take ``str`` argument also take optional ``i`` and ``j`` arguments (both
  integers). If ``i`` is passed, they act not on the whole ``str``, but on a substring of ``str``
  which starts at (1-based) index ``i``; if ``j`` is also passed, on a substring which starts at index
  ``i`` and ends at index ``j``. If ``j < i``, the substring is taken to be empty. If passed, ``i``
  must be positive, but ``j`` can be zero.

Plugins' and barlib's Lua functions
-----------------------------------
Plugins and barlibs can register Lua functions. They appear in ``luastatus.plugin`` and
``luastatus.barlib`` submodules, correspondingly.

Limitations
-----------
In luastatus, ``os.setlocale`` always fails as it is inherently not thread-safe.

SEPARATE STATE
==============
If ``widget.cb`` field has string type, it gets compiled as a function in a *separate state* (as if
with Lua's built-in ``loadstring`` function).
Whenever an event on such a widget occurs, the compiled function will be called in that state (not
in the widget's state, in which ``cb`` gets called).

This is useful for widgets that want not to receive data from a plugin, but to generate the data
themselves (possibly using some external modules). Such a widget may want to specify
::

   widget = {
       plugin = 'timer',
       opts = {period = 0},

and block in ``cb`` until it wants to update. The problem is that in this case, the widget's Lua
mutex is almost always being acquired by ``cb``, so the event handler has to wait until the next
update.
A separate-state ``event`` function solves that.

EXAMPLES
========
* ``luastatus-i3-wrapper alsa.lua time.lua``

  where ``alsa.lua`` is::

      widget = {
         plugin = 'alsa',
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

  and ``time.lua`` is::

      widget = {
         plugin = 'timer',
         opts = {period = 2},
         cb = function()
            return {full_text = os.date('[%H:%M]')}
         end,
      }

* ``luastatus -b dwm gmail.lua``

  where ``gmail.lua`` is::

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

More examples can be found in the ``examples/`` directory in the luastatus' git repository
(https://github.com/shdown/luastatus).

CHANGELOG
=========
See https://github.com/shdown/luastatus/releases.
