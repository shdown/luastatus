.. :X-man-page-only: luastatus-plugin-dbus-fn-prop
.. :X-man-page-only: #############################
.. :X-man-page-only:
.. :X-man-page-only: ###########################################################
.. :X-man-page-only: D-Bus plugin for luastatus: functions to get/set properties
.. :X-man-page-only: ###########################################################
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

This man page gives full descriptions of the functions to get/set properties of D-Bus objects belonging to
other programs.

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
The following functions for getting/setting properties of D-Bus objects are provided:

* ``luastatus.plugin.get_property(params)``

  Get a D-Bus property associated with an interface. ``params`` must be a table
  with the following fields, in addition to the "common" fields (all are **required**):

  - ``dest`` (string): a unique or well-known name of the owner of the property.

  - ``object_path`` (string): the path to the object to get a property of.

  - ``interface`` (string): the name of the interface.

  - ``property_name`` (string): the name of the property.

  On success, returns ``true, result``, where ``result`` is unmarshalled
  as described in section `Marshalling`_.

  On failure, returns ``false, err_msg, err_code``.

* ``luastatus.plugin.get_all_propertes(params)``

  Get all D-Bus properties associated with an interface. ``params`` must be the
  same as to ``get_property``, except that ``property_name`` should not be set.

  On success, returns ``true, result``, where ``result`` is the result of
  the ``GetAll`` call, unmarshalled as described in section `Marshalling`_.
  It looks like this::

    {{{"Property1", "Value1"}, {"Property2", "Value2"}}}

  On failure, returns ``false, err_msg, err_code``.

* ``luastatus.plugin.set_property(params)``

  Set a D-Bus property associated with an interface. ``params`` must be
  the same as to ``get_property``, except that a new field ``value`` must be set.
  ``value`` must be a D-Bus value object; it can be constructed with
  ``luastatus.plugin.dbustypes.mkval_from_fmt`` or
  ``luastatus.plugin.dbustypes.mkval_of_dtype``
  functions (see **luastatus-plugin-dbus-fn-mkval(7)** for more information).

  On success, returns ``true, result``, where ``result`` is an empty table (the ``Set``
  method does not return anything).

  On failure, returns ``false, err_msg, err_code``.

* ``luastatus.plugin.set_property_str(params)``

  Set D-Bus property associated with an interface, to a string. ``params`` must be
  the same as to ``get_property``, except that a new string field ``value_str``
  must be set.

  This function exists because the more generic function, presented above, is harder to
  use for simple use cases, and also for backward compatibility.

  On success, returns ``true, result``, where ``result`` is an empty table (the ``Set``
  method does not return anything).

  On failure, returns ``false, err_msg, err_code``.

Marshalling
===========
Please see the main man page (**luastatus-plugin-dbus**) or the main help file (``README.rst``),
section "D-Bus objects", for information about how values are converted from D-Bus objects
into Lua stuff.
