.. :X-man-page-only: luastatus-plugin-mpd
.. :X-man-page-only: #####################
.. :X-man-page-only:
.. :X-man-page-only: ########################
.. :X-man-page-only: mpd plugin for luastatus
.. :X-man-page-only: ########################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors state of an mpd server.

Options
=======
* ``hostname``: string

    Hostname to connect to. Default is to connect to the local host. An absolute path to a UNIX
    domain socket can also be specified (``port`` and ``bind`` are ignored then).

* ``port``: number

    Port to connect to. Default is 6600.

* ``password``: string

    Server's password.

* ``timeout``: number

    If specified and not negative, the number of seconds to wait before calling ``cb`` with
    ``what="timeout"`` again (after a connection has been established). May be fractional.

* ``retry_in``: number

    Number of seconds to retry in after the connection is lost. A negative value means do not retry
    and return immediately. May be fractional. Defaults to 10.

* ``retry_fifo``: string

    Path to an existent FIFO. The plugin does not create FIFO itself. To force a reconnect,
    ``touch(1)`` the FIFO, that is, open it for writing and then close.

* ``events``: array of strings

    List of MPD subsystems to poll for changes in. See the description of ``idle`` command at
    https://www.musicpd.org/doc/html/protocol.html#querying-mpd-s-status for the complete list.
    Default is ``{"mixer","player"}``.

* ``enable_tcp_keepalive``: bool

    Whether or not to enable TCP keepalive. Defaults to ``false``.
    This option is ignored if the plugin is configured to connect to a UNIX domain socket.

* ``bind``: table

    If provided, the plugin will bind the (TCP, not UNIX) socket to a specific address.
    The parameters for the binding are specified by this table.

    If the plugin is otherwise configured to connect to a UNIX domain socket (via ``hostname``
    option that starts with a slash), this option is ignored.

    If this option is provided, it must be a table with the following keys:

      - ``addr``: the value must be a string representing either IPv4 or IPv6 address.

      - ``ipver``: the value must be a string, either ``"ipv4"`` for IPv4 address or ``"ipv6"`` for IPv6 address.

    Note that there is no default for the ``ipver`` field, and there is no guessing what sort of
    address it is; if the ``bind`` table is present, but does not contain ``ipver`` key, the
    plugin will fail to initialize.


``cb`` argument
===============
A table with ``what`` entry.

* If ``what`` is ``"connecting"``, the plugin is now connecting to the server.

* If ``what`` is ``"update"``, either the plugin has just connected to the server and queried its
  state, or the server has just changed its state. Additionally, the following entries are provided:

  - ``song``: table with server's response to the ``currentsong`` command. Surprisingly, it is not
    documented at all, so here is an example (all values are strings):

    .. rst2man does not support tables with headers, so let's just use bold.

    +----------------------+-----------------------------+
    | **Key**              | **Value**                   |
    +----------------------+-----------------------------+
    | file                 | Sensou to Heiwa.mp3         |
    +----------------------+-----------------------------+
    | Last-Modified        | 2016-07-31T09:56:31Z        |
    +----------------------+-----------------------------+
    | Artist               | ALI PROJECT                 |
    +----------------------+-----------------------------+
    | AlbumArtist          | ALI PROJECT                 |
    +----------------------+-----------------------------+
    | Title                | 戦争と平和                  |
    +----------------------+-----------------------------+
    | Album                | Erotic & Heretic            |
    +----------------------+-----------------------------+
    | Track                | 8                           |
    +----------------------+-----------------------------+
    | Date                 | 2002-02-20                  |
    +----------------------+-----------------------------+
    | Genre                | J-Pop                       |
    +----------------------+-----------------------------+
    | Disc                 | 1/1                         |
    +----------------------+-----------------------------+
    | Time                 | 260                         |
    +----------------------+-----------------------------+
    | Pos                  | 0                           |
    +----------------------+-----------------------------+
    | Id                   | 4                           |
    +----------------------+-----------------------------+

  - ``status``: table with server's response to the ``status`` command. See the ``status`` command
    description at https://www.musicpd.org/doc/html/protocol.html#querying-mpd-s-status.

    All values are strings.

* It ``what`` is ``"timeout"``, the server hasn't changed its state for the number of seconds
  specified as the ``timeout`` option.

* If ``what`` is ``"error"``, the connection has been lost; the plugin is now going to sleep and try
  to reconnect. This is only reported if ``retry_in`` is non-negative.
