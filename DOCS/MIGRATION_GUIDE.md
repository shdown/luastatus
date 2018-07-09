0.2.0 to 0.3.0
===
* Semantics of the `greet` option of `inotify` plugin has changed;
  see `plugins/inotify/README.md` for details.

0.1.0 to 0.2.0
===
* `luastatus.spawn`, `luastatus.rc` and `luastatus.dollar` functions have been removed.
   Use `os.execute` and/or `io.popen` instead — and please don’t forget to properly escape the arguments:
    ````lua
    function shell_escape(s)
        return "'" .. s:gsub("'", "'\\''") .. "'"
    end
    ````

* `pipe` plugin has been removed. Use the `timer` plugin and `io.open` instead:
    ````lua
    f = io.popen('your command', 'r')
    wdiget = {
        plugin = 'timer',
        cb = function()
            local line = f:read('*line')
            -- ...
        end,
    }
    ````
