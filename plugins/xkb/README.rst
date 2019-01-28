.. :X-man-page-only: luastatus-plugin-xkb
.. :X-man-page-only: ####################
.. :X-man-page-only:
.. :X-man-page-only: ######################################
.. :X-man-page-only: X keyboard layout plugin for luastatus
.. :X-man-page-only: ######################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors current keyboard layout.

Options
=======
The following options are supported:

* ``display``: string

    Display to connect to. Default is to use ``DISPLAY`` environment variable.

* ``device_id``: number

    Keyboard device ID (as shown by ``xinput(1)``). Default is ``XkbUseCoreKbd``.

``cb`` argument
===============
A table with the following entries:

* ``name``: string

    Group name (if number of group names reported is sufficient).

* ``id``: number

    Group ID (0, 1, 2, or 3).
