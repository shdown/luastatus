.. :X-man-page-only: luastatus-plugin-dbus-fn-mkval
.. :X-man-page-only: #############################
.. :X-man-page-only:
.. :X-man-page-only: ############################################################
.. :X-man-page-only: D-Bus plugin for luastatus: functions to create D-Bus values
.. :X-man-page-only: ############################################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
The **dbus** plugin for **luastatus** provides functions to:

1. interact with other programs over D-Bus (calling methods and getting/setting properties);
2. construct D-Bus values and types from within Lua; this is needed for calling methods and setting properties.

For more information on **luastatus**, see **luastatus(1)**.
For more information on the **dbus** plugin for luastatus, see **luastatus-plugin-dbus(7)**.

This man page gives full descriptions of the functions to construct D-Bus values and types
from within Lua. All the functions described here are in the ``dbustypes`` module;
so please note that you should call them as ``luastatus.plugin.dbustypes.some_func()``.

Functions
=========
The following functions for construction of D-Bus values and types from within Lua are provided:

* ``luastatus.plugin.dbustypes.mkval_from_fmt(fmt, lua_val)``

  Make a D-Bus value object from the type string ``fmt`` and Lua representation
  ``lua_val``. For information on type strings, see `DBus type system and type strings`_.
  For information on the required structure of ``lua_val``, see `Value conversion`_.

* ``luastatus.plugin.dbustypes.mkval_of_dtype(dtype, lua_val)``

  Make a D-Bus value object of type ``dtype`` from Lua representation
  ``lua_val``. A ``dtype`` value can be obtained via a call to ``luastatus.plugin.dbustypes.parse_dtype``.
  For information on the required structure of ``lua_val``, see `Value conversion`_.

* ``luastatus.plugin.dbustypes.parse_dtype(fmt)``

  Make a D-Bus type object from the type string ``fmt``.
  For information on type strings, see `DBus type system and type strings`_.

DBus type system and type strings
=================
This section discusses the D-Bus type system and type strings.

In D-Bus, each type can be expressed as a type string, which is a printable
string, e.g. ``d`` or ``(susssasa{sv}i)``.
We will refer to the type string of a type as of *spelling* of that type.

For a more digestible description, the reader is advised to visit
<https://docs.gtk.org/glib/gvariant-format-strings.html> and/or
<https://dbus.freedesktop.org/doc/dbus-specification.html>.

Basic types
-----------
There are a number of *basic types*. The spelling of a basic type
is always a single letter:

* ``b``: boolean type.

* ``y``: byte type.

* ``n``, ``i``, ``x``: signed integer types, 16-bit, 32-bit and 64-bit, correspondingly.

* ``q``, ``u``, ``t``: unsigned integer types, 16-bit, 32-bit and 64-bit, correspondingly.

* ``d``: floating-point type (the same a Lua number).

* ``s``: string type; note that the string must be valid UTF-8, and must not contain NUL characters (``\0``).

* ``o``: D-Bus object path type. This is just like a string, but with additional requirements for the content.

* ``g``: D-Bus signature type. This is just like a string, but with additional requirements for the content.

* ``h``: "handle" type; it represents a file descriptor passed along with the call. There is no support for handles
  in this module; you can create the handle type, but you can't create a handle value object.

The variant type
----------------
There is a "variant" type; its spelling is ``v``.
It acts as a black box: you can put value of any type into a variant value,
but the type system only sees the variant type.
You can put a variant into a variant, too, so there may be many levels of variants.

It is frequently used to hide implementation-related details.

For example, the type ``a{sv}`` (array of dictionary entries from string to variant),
more conveniently "dictionary from strings to anything", is used in Desktop Notifications Specification
`for hints to notification daemon <https://specifications.freedesktop.org/notification/latest/protocol.html#id-1.10.3.3.4>`_.

Hints keys are always strings, but the value might be anything (see `the list of standard hints <https://specifications.freedesktop.org/notification/latest/hints.html>`_).
``suppress-sound`` is boolean, ``sound-file`` is string, ``x`` and ``y`` are int32. And also ``image-data`` is ``(iiibiiay)``.

Array types
-----------
There is a type that represents an array of something.
Each element in the array must be of the same type.
If the spelling of the element type is ``X``, then ``aX`` is the spelling of the type "array of values of type ``X``".

For example, ``as`` means array of strings, ``aad`` means an array of arrays of floating-point values.

Tuple types
-----------
While a value of type "array of string" might have any length,
the number of items in the tuple is fixed in the type signature.
Also, unlike arrays, tuple can contain values of different types.

So, a tuple can represent things like "three strings", "an int32 and a string" or "nothing at all" (empty tuple).

Tuple items don't have names; instead, they are referred to by their index.

The spelling of a tuple type consists of:
1. symbol ``(``, and
2. spellings of the types of all the items concatenated (without any delimiter), and
3. symbol ``)``.

So, "three strings" is spelled as ``(sss)``, "an int32 and a string" is spelled as "(is)",
and "nothing at all" is spelled as ``()``. The latter is called an empty tuple.

Dict entry types
----------------
The D-Bus type system don't have a dictionary type; instead, it emulates dictionaries
with arrays of *dict entries*. Basically, a dict entry is a pair of key and value,
with the requirement that the key must be of basic type.

Suppose we have two types, one with spelling ``K`` and anther with spelling ``V``.
We want to use the type with spelling ``K`` as the "key" type in a "dictionary",
and use the type with spelling ``V`` as the "value" type.
(Note that this can only be done if the "key" type is basic.)
Then the spelling of dict entry for this key-value pair of types is ``{KV}``.

For example, ``a{sv}`` means, literally, "array of dictionary entries from string to variant",
but in more human terms means "dictionary from strings to anything".

Value conversion
================
During the conversion from D-Bus object to Lua value, the conversion functions
require the following correspondence between D-Bus types and Lua values:

+------------------------+-------------------------+
| **D-Bus type string**  | **Lua representation**  |
+------------------------+-------------------------+
| ``b``                  | boolean                 |
+------------------------+-------------------------+
| ``y``                  | either string of length |
|                        | 1, or a number (numeric |
|                        | value of a byte)        |
+------------------------+-------------------------+
| ``n``, ``i``, ``x``,   | either a string or a    |
| ``q``, ``u``, ``t``    | number                  |
+------------------------+-------------------------+
| double                 | number                  |
+------------------------+-------------------------+
| string, object path,   | string                  |
| signature              |                         |
+------------------------+-------------------------+
| handle                 | special object with     |
|                        | value ``"handle"``      |
+------------------------+-------------------------+
| array, tuple           | array (a table with     |
|                        | numeric keys)           |
+------------------------+-------------------------+
| dict entry             | array of length 2: key  |
|                        | and value               |
+------------------------+-------------------------+

Creation of handles is not supported; it is possible to create a handle *type*,
but trying to create a handle *value* leads to an error being thrown.

Example
=======
This example shows a notification with text in red color::

    function show_notification()
        local summary = 'Hello'
        local body = 'World'

        local mkval = luastatus.plugin.dbustypes.mkval_from_fmt
        local args = mkval('(susssasa{sv}i)', {
            'luastatus',    -- appname
            0,              -- replaces_id
            "",             -- icon
            summary,        -- summary
            body,           -- body
            {},             -- actions
            {               -- hints
                {'fgcolor', mkval('s', '#ff0000')},
            },
            -1,             -- timeout
        })

        assert(luastatus.plugin.call_method({
            bus = 'session',
            dest = 'org.freedesktop.Notifications',
            object_path = '/org/freedesktop/Notifications',
            interface = 'org.freedesktop.Notifications',
            method = 'Notify',
            args = args,
        }))
    end

Notes
=====
There is also a module named ``dbustypes_lowlevel``.
It is not documented, because this module is probably of little interest to the user.
It is used by implementation of "high-level" ``dbustypes`` module.
