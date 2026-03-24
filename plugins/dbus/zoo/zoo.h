#pragma once

#include <lua.h>

struct Zoo;
typedef struct Zoo Zoo;

Zoo *zoo_new(void);

void zoo_register_funcs(Zoo *z, lua_State *L);

void zoo_destroy(Zoo *z);
