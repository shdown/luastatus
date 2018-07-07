Overview
===
luastatus provides the following function to barlibs and plugins:

```c
void ** (*map_get)(void *userdata, const char *key);
```

This function is **not** thread-safe and should only be used in the `init` function.

luastatus maintains a global mapping from zero-terminated strings to pointers (`void *`).
`map_get` returns a pointer to the pointer corresponding to the given key; if a map entry with the given key does not exist, it creates one with a null pointer value.

You can read and/or write from/to this pointer-to-pointer; it is guaranteed to be persistent across other calls to `map_get` and other functions.

Its intended use is for synchronization.

Lua interface
===

luastatus also provides `luastatus.map_get_handle(key)` function that returns a handle to the map entry. The handle has two methods:

  - `[str] = handle:read()`
      Reads the value, assuming it is either a null pointer or a C string. If it is neither, **the behaviour is undefined**.
      If it is a null pointer, returns `nil`; otherwise return the string.

  - `handle:write([str])`
      If `str` is absent or `nil`, writes a null pointer.
      Otherwise, makes a C string copy of a string `str` and writes its address.
      Note that there is no way to release the memory occupied by the copy, so be careful to not introduce memory leaks.

`luastatus.map_get_handle` is only available at widget initialization stage.
