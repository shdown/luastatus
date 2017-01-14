#ifndef ls_getenv_r_h_
#define ls_getenv_r_h_

// Thread-safe getenv. Pass environ as the second argument.
const char *
ls_getenv_r(const char *name, char *const *env);

#endif
