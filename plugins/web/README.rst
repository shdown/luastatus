.. :X-man-page-only: luastatus-plugin-web
.. :X-man-page-only: ####################
.. :X-man-page-only:
.. :X-man-page-only: ########################
.. :X-man-page-only: Web plugin for luastatus
.. :X-man-page-only: ########################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========

This plugin performs HTTP requests using libcurl. It is designed around a coroutine-based
"planner" mechanism that allows sequencing requests, sleeps, and callback updates.
It also provides utility functions for JSON encoding/decoding and URL encoding/decoding.

Options
=======

The following options are supported at plugin initialization:

* ``planner``: function (**required**)

  A Lua function that acts as a coroutine planner. It should yield tables describing
  the next action (see `Planner Actions`_).

  The most common planner would be make-request-and-sleep::

    function()
        while true do
            coroutine.yield({what = 'request', params = {...}})
            coroutine.yield({what = 'sleep', period = 5.0})
        end
    end

* ``with_headers_global``: boolean

  Whether to include HTTP response headers in all response callbacks. Defaults to false.

* ``debug_global``: boolean

  Whether to enable verbose libcurl debug logging globally. Defaults to false.

* ``make_self_pipe``: boolean

  If true, the ``wake_up()`` (see the `Functions`_ section) function will be available. Defaults to
  false.

Planner Actions
===============

The ``planner`` function should yield a table with an ``action`` key. The following actions are
supported:

* ``{action = "request", params = {...}}``

  Performs an HTTP request. The ``params`` table contains request-specific options (see
  `Request Options`_). Upon completion, the plugin calls ``cb`` with the response data.

* ``{action = "sleep", period = <number>}``

  Sleeps for ``period`` seconds. Upon completion, the coroutine is resumed with a boolean
  indicating whether it was woken up via ``wake_up()`` (true) or timed out (false).
  Note that this action doesn't call ``cb``.

* ``{action = "call_cb", what = <string>}``

  Immediately calls ``cb`` with a table containing only the ``what`` field, set to the
  specified string.

Request Options
===============

The ``params`` table in a ``request`` action supports the following options:

* ``url``: string (**required**)

  The URL to request.

* ``headers``: table

  An array of strings representing HTTP headers to send.

* ``timeout``: number

  Maximum time, in seconds, for the request. Zero means no timeout (this is the default).

* ``max_file_size``: integer

  Maximum file size to download. Zero means no limit (this is the default).
  Must be less than 2 Gb.

* ``auto_referer``: boolean

  Automatically set the Referer header when following Location headers.

* ``custom_request``: string

  Custom HTTP request method (e.g. DELETE).

* ``follow_location``: boolean

  Follow HTTP redirects.

* ``interface``: string

  Local interface to use for the connection.

* ``max_redirs``: integer

  Maximum number of redirects to follow.
  The value of -1 means unlimited number of redirects.
  The default depends on libcurl version; either unlimited or 30.

* ``tcp_keepalive``: boolean

  Enable TCP keepalive.

* ``proxy``, ``proxy_username``, ``proxy_password``: strings
  Proxy configuration.

* ``post_fields``: string

  Data to send in a POST (or POST-like) request.

* ``with_headers``: boolean

  Whether to include HTTP response headers. If either this option or the global
  ``with_headers_global`` option is enabled, headers are included.

* ``debug``: boolean

  Whether to enable verbose logging for this specific request. The logic is the same
  as with ``with_headers`` (see above).

cb argument
===========

The callback function ``cb`` is called with a table argument. The structure depends on the
action completed. Note that the ``sleep`` action doesn't call ``cb``.

* For ``request`` actions (HTTP requests has been done, regardless of status):

  A table ``{what = "response", status = <integer>, body = <string>, headers = <table (optional)>}``.

* For ``request`` actions (HTTP requests has **not** been done):

  A table ``{what = "response", status = 0, body = "", headers = {}, error = <string>}``.

* For ``call_cb`` actions:

  A table ``{what = <string>}``, where ``string`` is the value provided in the action.

Functions
=========

The following functions are provided in the plugin table:

* ``luastatus.plugin.wake_up()``

  Writes to the self-pipe. If the plugin is currently executing a ``sleep`` action, this
  interrupts the sleep and resumes the coroutine (``coroutine.yield`` then returns true).
  It doesn't directly call ``cb``.

  Waking up in the middle of a request is not supported; if a request is in flight,
  the wake-up will happen only after the request is done.

  Only available if the ``make_self_pipe`` option was set to ``true``; otherwise, it throws an
  error.

* ``luastatus.plugin.time_now()``

  Returns the current timestamp, as a floating-point number in seconds.
  If the system supports it, uses monotonic clock; otherwise, uses wall-clock.
  It may be helpful in case of a sleep interrupted by a wake-up.

* ``luastatus.plugin.get_supported_opts()``

  Returns a table listing all supported request options (keys are option names, values are ``true``).

* ``luastatus.plugin.json_decode(input, mark_arrays_vs_dicts, mark_nulls)``

  Decodes a JSON string into a Lua table. See the `JSON Decoding`_ section for details on the parameters.

* ``luastatus.plugin.json_encode_str(str)``

  Encodes a string into a JSON-safe string.
  Please note that it does **not** enclose the output in double quotes.

* ``luastatus.plugin.json_encode_num(num)``

  Converts a number into a JSON-conforming string representation.

* ``luastatus.plugin.urlencode(str[, plus_notation])``

  URL-encodes a string.
  ``plus_notation`` is an optional boolean parameter; if set to true, spaces will be replaced
  with ``+``, not ``%20``. ``plus_notation`` defaults to false.

* ``luastatus.plugin.urldecode(str)``
  URL-decodes a string. Returns nil on failure.


JSON Decoding
=============

The ``json_decode`` function provides options to handle nuances between JSON and Lua data structures:

* ``mark_arrays_vs_dicts``: boolean

  Lua tables are used for both JSON arrays and objects (dicts), making them indistinguishable
  by default. If this parameter is ``true``, the decoded tables will have a metatable set:

  + JSON arrays will have a metatable with ``is_array = true``.

  + JSON objects will have a metatable with ``is_dict = true``.

  This allows you to distinguish them using ``getmetatable(<table>).is_array`` or ``is_dict``.

* ``mark_nulls``: boolean

  In JSON, ``null`` is a valid value distinct from a missing key. In Lua, assigning ``nil`` to a
  table key removes the key. If this parameter is ``true``, JSON ``null`` values are decoded as
  a special light userdata object instead of Lua ``nil``. This allows you to distinguish between
  a key that is missing and a key that is explicitly set to ``null``. You can check for this
  special value using ``type(<value>) == "userdata"``.

Return value
------------
On success, returns the decoded Lua table. On failure, returns ``nil, err_msg``.

Limitations
-----------
The maximum nesting depth that the current implementation supports is 100.
If this limit is exceeded, the function fails and returns ``nil, "depth limit exceeded"``.
