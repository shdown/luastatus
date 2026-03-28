.. :X-man-page-only: luastatus-plugin-dbus
.. :X-man-page-only: #####################
.. :X-man-page-only:
.. :X-man-page-only: ##########################
.. :X-man-page-only: D-Bus plugin for luastatus
.. :X-man-page-only: ##########################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin subscribes to D-Bus signals.

Options
=======
The following options are supported:

* ``greet``: boolean

  Whether or not a first call to ``cb`` with ``what="hello"`` should be made as soon as the
  widget starts. Defaults to false.

* ``report_when_ready``: boolean

  Whether or not to call to ``cb`` with ``what="ready"`` once the plugin has subscribed to
  all events successfully.
  Note that there is no guarantee this will be done before any other call, including calls
  with ``what="signal"``, because glib has something called "priorities".
  Defaults to false.

* ``timeout``: number

  If specified and not negative, this plugin calls ``cb`` with ``what="timeout"`` if no D-Bus
  signal has been received in ``timeout`` seconds.

* ``signals``: array of tables

  Array of tables with the following entries (all are optional):

  - ``sender``: string

    Sender name to match on (unique or well-known name).

  - ``interface``: string

    D-Bus interface name to match on.

  - ``signal``: string

    D-Bus signal name to match on.

  - ``object_path``: string

    Object path to match on.

  - ``arg0``: string

    Contents of the first string argument to match on.

  - ``flags``: array of strings

    The following flags are recognized:

    + ``"match_arg0_namespace"``

      Match first arguments that contain a bus or interface name with the given namespace.

    + ``"match_arg0_path"``

      Match first arguments that contain an object path that is either equivalent to the
      given path, or one of the paths is a subpath of the other.

  - ``bus``: string

    Specify the bus to subscribe to the signal on: either ``"system"`` or ``"session"``;
    default is ``"session"``.

``cb`` argument
===============
A table with a ``what`` entry.

* If ``what`` is ``"hello"``, this is the first call to ``cb`` (only if the ``greet`` option was
  set to ``true``).

* If ``what`` is ``"ready"``, the plugin has subscribed to all the events successfully (only if
  the ``report_when_ready`` option was set to ``true``).

* It ``what`` is ``"timeout"``, a signal has not been received for the number of seconds specified
  as the ``timeout`` option.

* If ``what`` is ``"signal"``, a signal has been received. In this case, the table has the
  following additional entries:

  - ``bus``: string

    Either ``"session"`` or ``"system"``.

  - ``sender``: string

    Unique sender name.

  - ``object_path``: string

    Object path.

  - ``interface``: string

    D-Bus interface name.

  - ``signal``: string

    Signal name.

  - ``parameters``: *D-Bus object*

    Signal arguments.

D-Bus objects
=============
D-Bus objects are marshalled as follows:

.. rst2man does not support tables with headers, so let's just use bold.

+-----------------------+------------------------+
| **D-Bus object type** | **Lua representation** |
+-----------------------+------------------------+
| boolean               | boolean                |
+-----------------------+------------------------+
| byte, int16, uint16,  | string                 |
| int32, uint32, int64, |                        |
| uint64                |                        |
+-----------------------+------------------------+
| double                | number                 |
+-----------------------+------------------------+
| string, object path,  | string                 |
| signature             |                        |
+-----------------------+------------------------+
| handle                | special object with    |
|                       | value ``"handle"``     |
+-----------------------+------------------------+
| array, tuple, dict    | array                  |
| entry                 |                        |
+-----------------------+------------------------+

If an object cannot be marshalled, a special object with an error is generated instead.

Special objects
---------------
Special objects represent D-Bus objects that cannot be marshalled to Lua.

A special object is a function that, when called, returns either of:

* value;
* ``nil``, error.

Functions
=========

This plugin provides functions to:

1. interact with other programs over D-Bus (calling methods and getting/setting properties);
2. construct D-Bus values and types from within Lua; this is needed for calling methods and setting properties.

The full descriptions of all those functions would be a bit too long for a single man page.
So the detailed descriptions are provided in other man pages (or .rst files):

* **luastatus-plugin-dbus-fn-call-method(7)** (or ``README_FN_CALL_METHOD.rst`` file): functions to
  call methods of D-Bus objects:

  - ``luastatus.plugin.call_method``: calls a method of a D-Bus object.

  - ``luastatus.plugin.call_method_str``: same as above, but can only call methods accepting
    a single string argument or no arguments. It exists because it's easier to use it compared to
    the above, and also for backward compatibility.

* **luastatus-plugin-dbus-fn-prop(7)** (or ``README_FN_PROP.rst`` file): functions to get/set
  properties of D-Bus objects:

  - ``luastatus.plugin.get_property``: get a property of a D-Bus object.

  - ``luastatus.plugin.get_all_properties``: get all properties of a D-Bus object.

  - ``luastatus.plugin.set_property``: set a property of a D-Bus object.

  - ``luastatus.plugin.set_property_str``: same as above, but can only set string properties. It exists
    because it's easier to use it compared to the above, and also for backward compatibility.

* **luastatus-plugin-dbus-fn-mkval(7)** (or ``README_FN_MKVAL.rst`` file): functions to construct D-Bus
  values and types from within Lua:

  - module ``dbustypes``:

    + ``luastatus.plugin.dbustypes.mkval_from_fmt``: make a D-Bus value object from a type format string and a Lua value.

    + ``luastatus.plugin.dbustypes.parse_dtype``: parse a type format string into a D-Bus type object.

    + ``luastatus.plugin.dbustypes.mkval_of_dtype``: make a D-Bus value object from a D-Bus type object and a Lua value.

There is also a module named ``dbustypes_lowlevel``.
It is not documented, because this module is probably of little interest to the user.
It is used by implementation of "high-level" ``dbustypes`` module.
