This library injects a thread-safe pseudo-random number generator into a Lua
state's standard library, replacing math.random and math.randomseed functions.

It only does so if it's not LuaJIT and Lua version is less than 5.4:

  * LuaJIT and Lua >=5.4 implement their own lua_State-local PRNGs;

  * Lua 5.1/5.2 use rand()/srand() (not thread-safe; using from different
    threads without coordination leads to undefined behavior on e.g. FreeBSD).

  * Lua 5.3 apparently uses random()/srandom() on POSIX systems. This *seems*
    not to invoke undefined behavior directly (POSIX doesn't list these
    functions in the list of possibly-not-thread-safe functions), but using
    them from different threads without coordination doesn't make sense anyway:
    this way, the sequence is not reproducible, and seeding doesn't make sense.

As Zawinski said,

> An app for editing text becomes an IDE, then an OS. An app for displaying
> hypertext documents becomes a mail reader, then an OS.

So now we have to implement a PRNG and PRNG quality tests in a status bar
content generator. I wonder if eventually we will have to fork Lua or implement
our own programming language because of these thread safety issues or simply
the fact that Lua constantly breaks backward compatibility (fortunately, LuaJIT
doesn't).
