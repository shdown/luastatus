#ifndef call_state_h_
#define call_state_h_

#include <stddef.h>
#include <lua.h>
#include "libls/compdep.h"
#include "plugin_run_data.h"

typedef struct {
    lua_State *L;
    int L_inittop;
} CallState;

#define CALL_STATE_INITIALIZER ((CallState) {.L = NULL, .L_inittop = -1})

LS_INHEADER
void
call_state_call_begin(CallState *s, PluginRunData data)
{
    s->L = data.call_begin(data.pd->userdata);
    s->L_inittop = lua_gettop(s->L);
}

LS_INHEADER
void
call_state_call_end(CallState *s, PluginRunData data)
{
    data.call_end(data.pd->userdata);
    *s = CALL_STATE_INITIALIZER;
}

LS_INHEADER
void
call_state_call_begin_or_reuse(CallState *s, PluginRunData data)
{
    if (s->L) {
        lua_settop(s->L, s->L_inittop);
    } else {
        call_state_call_begin(s, data);
    }
}

#endif
