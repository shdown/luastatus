#ifndef barlib_h_
#define barlib_h_

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include "include/barlib_data.h"

typedef enum {
    BARLIB_STATE_LOADED,
    BARLIB_STATE_INITED,
} BarlibState;

typedef struct {
    LuastatusBarlibIface iface;
    LuastatusBarlibData data;
    pthread_mutex_t set_mtx;
    void *dlhandle;
    BarlibState state;
} Barlib;

bool
barlib_load(Barlib *b, const char *filename);

bool
barlib_init(Barlib *b, LuastatusBarlibData data, const char *const *opts, size_t nwidgets);

void
barlib_uninit(Barlib *b);

void
barlib_unload(Barlib *b);

#endif
