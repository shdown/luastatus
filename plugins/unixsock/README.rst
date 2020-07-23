.. :X-man-page-only: luastatus-plugin-unixsock
.. :X-man-page-only: #########################
.. :X-man-page-only:
.. :X-man-page-only: #######################################
.. :X-man-page-only: UNIX domain socket plugin for luastatus
.. :X-man-page-only: #######################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin acts as a UNIX domain socket server, accpeting incoming connections and making a call
when a client produces a line.

In order to have it make a call, connect to the UNIX socket and send a line to it.
From a command line, this can be done with, e.g.::

    echo some_data | socat stdio UNIX-CONNECT:/tmp/my-unix-socket

(This is an experimental plugin; it is not enabled by default and not yet built by the Gentoo/Debian
build scripts provided in this repo.)

Options
=======
The following options are supported:

* ``path``: string

  Filesystem path to the UNIX domain socket. This option is required.

* ``try_unlink``: boolean

  Whether or not to try to unlink (remove) the socket file before creating a server.
  Defaults to true.

* ``timeout``: number

  If specified and not negative, this plugin calls ``cb`` with ``what="timeout"`` if no client
  has produced a line in ``timeout`` seconds.

* ``greet``: boolean

  Whether or not to call ``cb`` with ``what="hello"`` as soon as the plugin starts. Defaults to
  false.

* ``max_concur_conns``: number

  Specifies the maximum number of concurrent connections: once the number of connected clients
  reaches this value, others will be forced to wait in a queue.

  Please do not use this option unless you know what you are doing: too many concurrent connections
  will eat up file descriptors from the whole luastatus process.

  Defaults to 5, which is more than enough if the clients are not buggy.

``cb`` argument
===============
A table with ``what`` entry:

* if it is ``"hello"``, the function is being called for the first time (only if the ``greet``
  option was set to ``true``);

* if it is ``"timeout"``, the function has not been called for the number of seconds specified as
  the ``timeout`` option;

* if it is ``"line"``, a client has produced a line; in this case, the table also has ``line``
  entry with string value.

Functions
=========
The following functions are provided:

* ``luastatus.plugin.push_timeout(seconds)``

  Changes the timeout for one iteration.
