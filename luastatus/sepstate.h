#ifndef sepstate_h_
#define sepstate_h_

#include <lua.h>
#include <pthread.h>

typedef struct {
    lua_State *L;
    pthread_mutex_t lua_mtx;
} SepState;

extern SepState sepstate;

void
sepstate_ensure_inited(void);

#endif
