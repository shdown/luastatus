#ifndef rules_names_h_
#define rules_names_h_

#include <X11/Xlib.h>
#include <stdbool.h>

typedef struct {
    const char *rules;
    const char *model;
    const char *layout;
    const char *options;
    unsigned char *_data;
} RulesNames;

bool
rules_names_load(Display *dpy, RulesNames *out);

void
rules_names_destroy(const RulesNames *rn);

#endif
