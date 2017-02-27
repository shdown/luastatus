`libls/`: unless stated otherwise, a function is thread-safe and not reentrant.

  * `ls_close(fd)`, where `fd < 0`, does nothing (fails with `EBADFD`).

  * `free(NULL)` does nothing.
