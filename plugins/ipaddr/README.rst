.. :X-man-page-only: luastatus-plugin-ipaddr
.. :X-man-page-only: #######################
.. :X-man-page-only:
.. :X-man-page-only: ###############################
.. :X-man-page-only: IP address plugin for luastatus
.. :X-man-page-only: ###############################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin monitors the IP addresses used for outgoing connections by various network interfaces.

Options
=======
The following options are supported:

* ``warn``: boolean

    Produce a warning if an IP address for an existing network interface cannot be obtained.

``cb`` argument
===============
A table with ``ipv4`` and ``ipv6`` entries, which are also tables whose keys are interface names and values are IP strings.
