#include "taints.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "include/loglevel.h"
#include "libls/vector.h"
#include "libls/compdep.h"
#include "log.h"
#include "widget.h"
#include "barlib.h"

static const size_t TAINT_OWNER_BARLIB = SIZE_MAX;

typedef struct {
    const char *id;
    size_t owner;
} Taint;

typedef LS_VECTOR_OF(Taint) TaintsVector;

static inline
int
taint_cmp(const void *a, const void *b)
{
    return strcmp(((const Taint *) a)->id, ((const Taint *) b)->id);
}

static
void
push_taints(TaintsVector *tv, const char *const *taints, size_t owner)
{
    if (!taints) {
        return;
    }
    for (const char *const *s = taints; *s; ++s) {
        LS_VECTOR_PUSH(*tv, ((Taint) {.id = *s, .owner = owner}));
    }
}

static
void
log_taint_block_begin(const char *id)
{
    internal_logf(LUASTATUS_INFO, "the following entities share taint '%s':", id);
}

static
void
log_taint_owner_barlib(LS_ATTR_UNUSED_ARG Barlib *b)
{
    internal_logf(LUASTATUS_INFO, "  * the barlib");
}

static
void
log_taint_owner_widget(Widget *w)
{
    internal_logf(LUASTATUS_INFO, "  * plugin '%s' requested by widget '%s'", w->plugin.name,
                  w->filename);
}

bool
check_taints(Barlib *barlib, Widget *widgets, size_t nwidgets)
{
    TaintsVector tv = LS_VECTOR_NEW();
    bool ret = true;

    push_taints(&tv, barlib->iface.taints, TAINT_OWNER_BARLIB);
    for (size_t i = 0; i < nwidgets; ++i) {
        if (widgets[i].state != WIDGET_STATE_DUMMY) {
            push_taints(&tv, widgets[i].plugin.iface.taints, i);
        }
    }

    // see ../DOCS/empty-ranges-and-c-stdlib.md
    if (tv.size) {
        qsort(tv.data, tv.size, sizeof(Taint), taint_cmp);
    }

    for (size_t i = 1; i < tv.size; ++i) {
        if (taint_cmp(&tv.data[i - 1], &tv.data[i]) == 0) {
            ret = false;
            size_t from = i;
            if (i == 1 || taint_cmp(&tv.data[i - 2], &tv.data[i - 1]) != 0) {
                log_taint_block_begin(tv.data[i].id);
                --from;
            }
            for (size_t j = from; j <= i; ++j) {
                if (tv.data[j].owner == TAINT_OWNER_BARLIB) {
                    log_taint_owner_barlib(barlib);
                } else {
                    log_taint_owner_widget(&widgets[tv.data[j].owner]);
                }
            }
        }
    }

    LS_VECTOR_FREE(tv);
    return ret;
}
