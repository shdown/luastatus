.. :X-man-page-only: luastatus-plugin-sway-xkb
.. :X-man-page-only: #########################
.. :X-man-page-only:
.. :X-man-page-only: ############################################
.. :X-man-page-only: Sway WM keyboard layout plugin for luastatus
.. :X-man-page-only: ############################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors the current keyboard layout.
It is specific to Sway WM (https://swaywm.org/), because Wayland currently provides no
compositor-agnostic means to query or subscribe to changes of keyboard layout.

Options
=======
The following options are supported:

* ``socket_path``: string (optional)

Specify custom Sway WM socket path. Default is to auto-detect via ``SWAYSOCK`` environment
variable or output of ``sway --get-socketpath`` command.

* ``report_all_names``: boolean (optional)

Whether to also report the list of all keyboard layouts configured for a keyboard.
Defaults to false.

* ``debug``: boolean (optional)

Whether to log all the traffic between the plugin and Sway WM. Defaults to false.

* ``no_env_var``: boolean (optional)

When auto-detecting the socket path, skip querying the ``SWAYSOCK`` environment variable
and proceed with spawning ``sway --get-socket-path`` command.
Normally should only be used for debugging.
Defaults to false.

``cb`` argument
===============
An array where each element is a table with the following entries (corresponding to a keyboard device):

* ``ident`` (string): keyboard identifier

* ``active_name`` (string): name of the active keyboard layout

* ``active_idx`` (integer): index of the active keyboard layout

* ``all_names`` (array of strings, optional): list of all keyboard layouts configured for this keyboard.
  Only present if ``report_all_names`` option was set to true.
