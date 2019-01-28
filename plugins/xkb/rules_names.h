#ifndef rules_names_h_
#define rules_names_h_

#include <X11/Xlib.h>
#include <stdbool.h>

typedef struct {
    // These all can be /NULL/.
    const char *rules;
    const char *model;
    const char *layout;
    const char *options;

    // Private data; do not touch.
    unsigned char *data_;
} RulesNames;

bool
rules_names_load(Display *dpy, RulesNames *out);

void
rules_names_destroy(RulesNames *rn);

#endif
