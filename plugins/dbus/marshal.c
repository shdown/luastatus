#include "marshal.h"

#include "libls/compdep.h"
#include "libls/panic.h"

#include <stdio.h>
#include <limits.h>
#include <inttypes.h>

static
int
l_special_object(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1)); // L: upvalue1
    if (lua_isnil(L, -1)) {
        lua_pushvalue(L, lua_upvalueindex(2)); // L: upvalue1 upvalue 2
        return 2;
    } else {
        // L: upvalue 1
        return 1;
    }
}

// forward declaration
static
void
push_gvariant(lua_State *L, GVariant *var, unsigned recurlim);

static
void
on_recur_lim(lua_State *L)
{
    lua_pushnil(L); // L: nil
    lua_pushstring(L, "depth limit exceeded"); // L: nil str
    lua_pushcclosure(L, l_special_object, 2); // L: closure
}

static
void
push_gvariant_strlike(lua_State *L, GVariant *var)
{
    gsize ns;
    const gchar *s = g_variant_get_string(var, &ns);
    lua_pushlstring(L, s, ns);
}

static
void
push_gvariant_iterable(lua_State *L, GVariant *var, unsigned recurlim)
{
    if (!recurlim--) {
        on_recur_lim(L);
        return;
    }

    GVariantIter iter;
    const int n = g_variant_iter_init(&iter, var);
    lua_createtable(L, n, 0); // L: table

    GVariant *elem;
    for (int i = 1; (elem = g_variant_iter_next_value(&iter)); ++i) {
        push_gvariant(L, elem, recurlim); // L: table value
        lua_rawseti(L, -2, i); // L: table
        g_variant_unref(elem);
    }
}

static
void
push_gvariant(lua_State *L, GVariant *var, unsigned recurlim)
{
    if (!recurlim--) {
        on_recur_lim(L);
        return;
    }

#define PUSH_STR(Fmt_, What_) \
    do { \
        char buf_[32]; \
        snprintf(buf_, sizeof(buf_), Fmt_, What_); \
        lua_pushstring(L, buf_); \
    } while (0)

    switch (g_variant_classify(var)) {
        case G_VARIANT_CLASS_BOOLEAN:
            lua_pushboolean(L, g_variant_get_boolean(var));
            break;

        case G_VARIANT_CLASS_BYTE:
            PUSH_STR("%u", (unsigned) g_variant_get_byte(var));
            break;

        case G_VARIANT_CLASS_INT16:
            PUSH_STR("%" G_GINT16_FORMAT, g_variant_get_int16(var));
            break;

        case G_VARIANT_CLASS_UINT16:
            PUSH_STR("%" G_GUINT16_FORMAT, g_variant_get_uint16(var));
            break;

        case G_VARIANT_CLASS_INT32:
            PUSH_STR("%" G_GINT32_FORMAT, g_variant_get_int32(var));
            break;

        case G_VARIANT_CLASS_UINT32:
            PUSH_STR("%" G_GUINT32_FORMAT, g_variant_get_uint32(var));
            break;

        case G_VARIANT_CLASS_INT64:
            PUSH_STR("%" G_GINT64_FORMAT, g_variant_get_int64(var));
            break;

        case G_VARIANT_CLASS_UINT64:
            PUSH_STR("%" G_GUINT64_FORMAT, g_variant_get_uint64(var));
            break;

        case G_VARIANT_CLASS_DOUBLE:
            lua_pushnumber(L, g_variant_get_double(var));
            break;

        case G_VARIANT_CLASS_STRING:
        case G_VARIANT_CLASS_OBJECT_PATH:
        case G_VARIANT_CLASS_SIGNATURE:
            push_gvariant_strlike(L, var);
            break;

        case G_VARIANT_CLASS_VARIANT:
            push_gvariant(L, g_variant_get_variant(var), recurlim);
            break;

        case G_VARIANT_CLASS_MAYBE:
            {
                GVariant *maybe = g_variant_get_maybe(var);
                if (maybe) {
                    push_gvariant(L, maybe, recurlim);
                } else {
                    lua_pushstring(L, "nothing"); // L: str
                    lua_pushcclosure(L, l_special_object, 1); // L: closure
                }
            }
            break;

        case G_VARIANT_CLASS_ARRAY:
        case G_VARIANT_CLASS_TUPLE:
        case G_VARIANT_CLASS_DICT_ENTRY:
            push_gvariant_iterable(L, var, recurlim);
            break;

        case G_VARIANT_CLASS_HANDLE:
        default:
            {
                const GVariantType *type = g_variant_get_type(var);
                const gchar *s = g_variant_type_peek_string(type);
                const gsize ns = g_variant_type_get_string_length(type);
                lua_pushlstring(L, s, ns); // L: str
                lua_pushcclosure(L, l_special_object, 1); // L: closure
            }
            break;
    }
#undef PUSH_STR
}

void
marshal(lua_State *L, GVariant *var)
{
    if (!var) {
        lua_pushnil(L);
        return;
    }

    if (lua_checkstack(L, 510)) {
        push_gvariant(L, var, 500);
    } else {
        lua_pushnil(L); // L: nil
        lua_pushstring(L, "out of memory"); // L: nil str
        lua_pushcclosure(L, l_special_object, 2); // L: closure
    }
}
