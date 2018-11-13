.. :X-man-page-only: luastatus-plugin-imap
.. :X-man-page-only: #####################
.. :X-man-page-only:
.. :X-man-page-only: ###########################
.. :X-man-page-only: IMAPv4 plugin for luastatus
.. :X-man-page-only: ###########################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This derived plugin monitors the number of unread mails in an IMAP mailbox.

Functions
=========

* ``widget(tbl)``

    Constructs a ``widget`` table required by luastatus. ``tbl`` is a table with the following
    fields:

    **(required)**

    - ``cb``: function

        The callback that will be called with the number of unread mails, or with ``nil`` if it is
        unknown.

    - ``host``: string

        Host name.

    - ``port``: number

        Port number.

    - ``login``, ``password``: strings

        Your credentials.

    - ``mailbox``: string

        Mailbox name (typically ``"Inbox"``).

    - ``error_sleep_period``: number

        Number of seconds to sleep after an error.

    **(optional)**

    - ``use_ssl``: boolean

        Whether to use SSL (you probably should set it to ``true``). Defaults to false.

    - ``verbose``: boolean

        Whether to be verbose (useful for troubleshooting). Default to false.

    - ``timeout``: number

        Idle timeout, in seconds, after which to reconnect; you probably want to set this to
        something like ``5 * 60``.

    - ``handshake_timeout``: number

        SSL handshake timeout, in seconds.

    - ``event``

        The ``event`` entry of the resulting table (see ``luastatus`` documentation for the
        description of ``widget.event`` field).
