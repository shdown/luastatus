.. :X-man-page-only: luastatus-plugin-dbus-fn-call-method
.. :X-man-page-only: ####################################
.. :X-man-page-only:
.. :X-man-page-only: #####################################################
.. :X-man-page-only: D-Bus plugin for luastatus: functions to call methods
.. :X-man-page-only: #####################################################
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

This man page gives full descriptions of the functions to call methods of D-Bus objects belonging to other programs.

All the functions take exactly one argument, which must be a table with string keys (we are going to refer to
these keys as *fields*).

Common fields
=============

There are a number of fields that are accepted by all the functions listed here:

* ``bus`` (string, **required**): the bus to use: either ``"system"`` or ``"session"``.

* ``flag_no_autostart`` (boolean, optional): whether to pass
  ``G_DBUS_CALL_FLAGS_NO_AUTO_START`` flag (which means don't launch an
  owner for the destination name in response to this method invocation).

  Defaults to false.

* ``timeout`` (number, optional): timeout to wait for the reply for, in seconds.
  Default is to use the proxy default timeout.
  Pass ``math.huge`` to wait forever.

Functions
=========
The following functions for calling methods of D-Bus objects are provided:

* ``luastatus.plugin.call_method(params)``

  Call a D-Bus method with the specified arguments.

  ``params`` must be a table with the following fields, in addition to the "common" fields:

  - ``dest`` (string): a unique or well-known name of the owner.

  - ``object_path`` (string): the path to the object.

  - ``interface`` (string): the name of the interface.

  - ``method`` (string): the name of the method.

  - ``args`` (D-Bus value object): the arguments for the call, as a tuple. The tuple can be
    constructed with ``luastatus.plugin.dbustypes.mkval_from_fmt``
    or ``luastatus.plugin.dbustypes.mkval_of_dtype``
    functions (see **luastatus-plugin-dbus-fn-mkval(7)** for more information).

  On success, returns ``true, result``, where ``result`` is the result of
  the call call, unpacked as described in section `Marshalling`_.

  On failure, returns ``false, err_msg, err_code``.

* ``luastatus.plugin.call_method_str(params)``

  Call a D-Bus method, either with no arguments or a single string argument.

  This function exists because the more generic function, presented above, is harder to
  use for simple use cases, and also for backward compatibility.

  ``params`` must be a table with the following fields, in addition to the "common" fields:

  - ``dest`` (string): a unique or well-known name of the owner.

  - ``object_path`` (string): the path to the object.

  - ``interface`` (string): the name of the interface.

  - ``method`` (string): the name of the method.

  - ``arg_str`` (optional, string): the argument for the call. If specified, the method
    will be called with a single string argument. If not specified, the method will be
    called with no arguments.

  On success, returns ``true, result``, where ``result`` is the result of
  the call call, unpacked as described in section `Marshalling`_.

  On failure, returns ``false, err_msg, err_code``.

Marshalling
===========
Please see the main man page (**luastatus-plugin-dbus**) or the main help file (``README.rst``),
section "D-Bus objects", for information about how values are converted from D-Bus objects
into Lua stuff.
