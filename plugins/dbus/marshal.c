#include "marshal.h"

#include "libls/compdep.h"

#include <stdio.h>
#include <limits.h>
#include <inttypes.h>

// forward declaration
static
void
push_gvariant(lua_State *L, GVariant *var, unsigned recurlim);

static
void
on_recur_lim(lua_State *L)
{
    lua_pushstring(L, "(recursion limit reached)");
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

    lua_newtable(L); // L: table
    GVariantIter iter;
    g_variant_iter_init(&iter, var);
    GVariant *elem;
    for (int n = 1; (elem = g_variant_iter_next_value(&iter)); ++n) {
        push_gvariant(L, elem, recurlim); // L: table value
        lua_rawseti(L, -2, n); // L: table
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

    if (!var) {
        lua_pushnil(L);
        return;
    }

    const char *kind;
    lua_newtable(L); // L: ? table

#define ALTSTR(GType_, LuaType_, CastTo_, Fmt_) \
    do { \
        char buf_[32]; \
        snprintf(buf_, sizeof(buf_), Fmt_, (CastTo_) g_variant_get_ ## GType_(var)); \
        lua_pushstring(L, buf_); /* L: ? table str */ \
        lua_setfield(L, -2, "as_string"); /* L: ? table */ \
        lua_push ## LuaType_(L, g_variant_get_ ## GType_(var)); /* L: ? table value */ \
    } while (0)

    switch (g_variant_classify(var)) {
        case G_VARIANT_CLASS_BOOLEAN:
            kind = "boolean";
            lua_pushboolean(L, g_variant_get_boolean(var));
            break;
        case G_VARIANT_CLASS_BYTE:
            kind = "byte";
            ALTSTR(byte, integer, unsigned, "%u");
            break;
        case G_VARIANT_CLASS_INT16:
            kind = "int16";
            ALTSTR(int16, integer, int, "%d");
            break;
        case G_VARIANT_CLASS_UINT16:
            kind = "uint16";
            ALTSTR(uint16, integer, unsigned, "%u");
            break;
        case G_VARIANT_CLASS_INT32:
            kind = "int32";
            ALTSTR(int32, integer, int, "%d");
            break;
        case G_VARIANT_CLASS_UINT32:
            kind = "uint32";
            ALTSTR(uint32, integer, unsigned, "%u");
            break;
        case G_VARIANT_CLASS_INT64:
            kind = "int64";
            ALTSTR(int64, integer, int64_t, "%" PRId64);
            break;
        case G_VARIANT_CLASS_UINT64:
            kind = "uint64";
            ALTSTR(uint64, integer, uint64_t, "%" PRIu64);
            break;
        case G_VARIANT_CLASS_HANDLE:
            kind = "handle";
            lua_pushnil(L);
            break;
        case G_VARIANT_CLASS_DOUBLE:
            kind = "double";
            ALTSTR(double, number, double, "%g");
            break;
        case G_VARIANT_CLASS_STRING:
            kind = "string";
            push_gvariant_strlike(L, var);
            break;
        case G_VARIANT_CLASS_OBJECT_PATH:
            kind = "object_path";
            push_gvariant_strlike(L, var);
            break;
        case G_VARIANT_CLASS_SIGNATURE:
            kind = "signature";
            push_gvariant_strlike(L, var);
            break;
        case G_VARIANT_CLASS_VARIANT:
            kind = "variant";
            push_gvariant(L, g_variant_get_variant(var), recurlim);
            break;
        case G_VARIANT_CLASS_MAYBE:
            kind = "maybe";
            push_gvariant(L, g_variant_get_maybe(var), recurlim);
            break;
        case G_VARIANT_CLASS_ARRAY:
            kind = "array";
            push_gvariant_iterable(L, var, recurlim);
            break;
        case G_VARIANT_CLASS_TUPLE:
            kind = "tuple";
            push_gvariant_iterable(L, var, recurlim);
            break;
        case G_VARIANT_CLASS_DICT_ENTRY:
            kind = "dict_entry";
            {
                GVariantIter iter;
                if (g_variant_iter_init(&iter, var) == 2) {
                    GVariant *key = g_variant_iter_next_value(&iter);
                    // L: ? table
                    push_gvariant(L, key, recurlim); // L: ? table key
                    lua_setfield(L, -2, "key"); // L: ? table
                    g_variant_unref(key);

                    GVariant *value = g_variant_iter_next_value(&iter);
                    push_gvariant(L, value, recurlim);
                    g_variant_unref(value);
                } else {
                    // Not sure if this is possible, but whatever.
                    push_gvariant_iterable(L, var, recurlim);
                }
            }
            break;
        default:
            LS_UNREACHABLE();
    }
    // L: ? table value
    lua_setfield(L, -2, "value"); // L: ? table
    lua_pushstring(L, kind); // L: ? table string
    lua_setfield(L, -2, "kind"); // L: ? table
#undef ALTSTR
}

void
marshal(lua_State *L, GVariant *var)
{
    if (lua_checkstack(L, 510)) {
        push_gvariant(L, var, 500);
    } else {
        lua_pushstring(L, "(out of memory)");
    }
}
