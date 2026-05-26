Overview
===

Whenever we export C functions to Lua, we need to guarantee we don't leak any
resources and don't end up in an inconsistent state (e.g. with a mutex locked)
if any Lua function throw (i.e., perform a longjmp).

Examples of Lua functions that can throw:

  * `lua_pop()` and `lua_settop()` in Lua 5.4+.

  * Functions that create new garbage-collected objects:
  `lua_pushstring()`/`lua_pushlstring()`, `lua_newtable()`/`lua_createtable()`,
  `lua_newthread()`, `lua_pushcclosure()` with # of upvalues > 0,
  `lua_newuserdata()`/`lua_newuserdatauv()`.

  * Functions that do table manipulation: `lua_getfield()`/`lua_setfield()`,
  `lua_geti()`/`lua_seti()`, `lua_gettable()`/`lua_settable()`.

  * `lua_next()`.

  * `lua_rawseti()`.

  * Functions that manipulate globals: `lua_getglobal()`/`lua_setglobal()`.

  * `luaL_ref()`.

Mandatory notation (enforced)
===

To ensure we do this, each call to `lua_pushcfunction()`, `lua_pushcclosure()`,
or module-specific functions/macros that create a registry entry, must be
prepended with string `/*__OK__*/`, which means that the author checked that
this C function handles long jumps by Lua API correctly. This is enforced by
`check_forbidden.sh` script in the root of the repo.

Optional notation (not enforced)
===

Moreover, it is recommended that each `lua_CFunction` declaration or definition
be marked with a comment that describes the function's semantics in relation
to long jumps by Lua API. The comment should be placed after the closing `)` in
argument list. The following "marks" are currently defined:

  * `/*__THROWABLE__*/`: this function may throw, and this is fine; in this
  case, it does not leak anything or end up in an inconsistent state.

  * `/*__FATAL_IF_THROWS__*/`: this function may throw, but the caller must
  ensure that the whole program is immediately terminated if it does so. Mainly
  used by functions that can only throw on out-of-memory condition.

  * `/*__PASS_THRU_IF_THROWS__*/`: this function may throw; the caller must
  catch any error, clean up resources or fix inconsistent state, and re-throw
  the error.

  * `/*__MUST_NEVER_THROW__*/`: this function never throws. Should be used for,
  e.g., implementations of `__gc` metamethod.

  * `/*__TERMINATES_PROGRAM__*/`: this function always terminates the whole
  program; it never returns or throws. Should be used for panic handlers.
