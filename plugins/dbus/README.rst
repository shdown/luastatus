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
========
The following options are supported:

    * ``greet``: boolean

        Whether or not a first call to ``cb`` with ``what="hello"`` should be made as soon as the
        widget starts. Defaults to false.

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

        -  ``object_path``: string

             Object path to match on.

        - ``arg0``: string

            Contents of first string argument to match on.

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
| maybe                 | as-is, or as a         |
|                       | special object with    |
|                       | value ``"nothing"``    |
+-----------------------+------------------------+
| handle                | special object with    |
|                       | value ``"handle"``     |
+-----------------------+------------------------+
| array, tuple, dict    | array                  |
| entry                 |                        |
+-----------------------+------------------------+

If an object cannot be marshalled, a special object with an error is generated instead.

Special objects
===============
Special objects represent D-Bus objects that cannot be marshalled to Lua.

A special object is a function that, when called, returns either of:

* value;
* ``nil``, error.
