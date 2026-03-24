/*
 * Copyright (C) 2026  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

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
