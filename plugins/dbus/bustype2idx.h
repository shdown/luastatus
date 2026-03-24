#pragma once

#include <stddef.h>
#include <gio/gio.h>
#include "libls/ls_compdep.h"
#include "libls/ls_panic.h"

LS_INHEADER size_t bustype2idx(GBusType bus_type)
{
    switch (bus_type) {
    case G_BUS_TYPE_SYSTEM:
        return 0;
    case G_BUS_TYPE_SESSION:
        return 1;
    default:
        LS_PANIC("bustype2idx: got unexpected GBusType value");
    }
}
