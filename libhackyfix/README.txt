Lua uses non-thread-safe functions all over the place and does another things
that are very problematic in multi-threaded environments.

Examples of things that are very problematic in multi-threaded environments:
  * Calling "fflush(NULL)" before calling popen() in its implementation of
    io.popen() function. The reasons why Lua does this are unknown to us.

    fflush(NULL) traverses all open FILE* objects, including read-only ones,
    acquires the lock on each one, flushes it, and then releases the lock.

    The problem is that "fflush(NULL)" hangs if, at the time of the call, at
    least one FILE* object is waiting on an input (e.g. blocked in fgetc(),
    getline() or similar) or is blocked on writing (e.g. when writing to a
    pipe).

    It only returns once all blocking stdio functions that are in-flight
    return, because all stdio functions (except unlocked_* ones) also lock the
    file.

    This makes using stdio in any of the threads very risky: Lua's io.popen()
    in another thread now may block for indefinite time. Note that Lua's stdlib
    also uses stdio all over the place.

Examples of usage of non-thread-safe (according to POSIX-2008) functions:
  * exit(): only used in implementation of os.exit().
    - we replace this Lua function in luastatus.

  * setlocale(): only used in implementation of os.setlocale().
    - we replace this Lua function in luastatus.

  * system(): only used in implementation of os.execute().
    - we replace this Lua function in luastatus.

  * dlerror(): used internally!

  * getenv(): used internally! And also in implementation of os.getenv().

  * strerror(): used internally!

So this library replaces the following functions with thread-safe counterparts
and/or different semantics to work around things that are very problematic in
multi-threaded environments:

  * fflush();

  * strerror();

  * getenv();

  * dlerror() (unless compiled with glibc, where it *is* thread-safe).
