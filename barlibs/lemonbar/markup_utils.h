#ifndef markup_utils_h_
#define markup_utils_h_

#include <lua.h>
#include <stddef.h>

#include "libls/string_.h"

void
push_escaped(lua_State *L, const char *s, size_t ns);

void
append_sanitized_b(LSString *buf, size_t widget_idx, const char *s, size_t ns);

const char *
parse_command(const char *line, size_t nline, size_t *ncommand, size_t *widget_idx);

#endif
