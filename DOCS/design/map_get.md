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
