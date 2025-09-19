.. :X-man-page-only: luastatus-plugin-llamacxx
.. :X-man-page-only: #########################
.. :X-man-page-only:
.. :X-man-page-only: ################################################################
.. :X-man-page-only: Responses from a local LLM (with llama.cpp) plugin for luastatus
.. :X-man-page-only: ################################################################
.. :X-man-page-only:
.. :X-man-page-only: :Copyright: LGPLv3
.. :X-man-page-only: :Manual section: 7

Overview
========
This plugin provides answers to the given prompts from a local LLM (large language
model). It uses `llama.cpp <https://github.com/ggml-org/llama.cpp/>`_ REST API.
A widget using this plugin can either generate the prompts by itself (e.g.
fetch a random prompt from a list using Lua's ``math.random``) or use data
from "nested widgets" (e.g. ``xtitle`` or ``mpd``) that this plugin can spawn.

Options
=======
The following options are supported:

* ``connection``: table

  Connection settings; see the `Connection settings`_ subsection below for more information.

* ``prompt``: function (required)

  Function to generate a prompt. Must take a single argument: a table whose
  keys are ``data_sources``' keys and values are data from the corresponding
  data sources (the table is empty if ``data_sources`` is empty or ``nil``).

  A value behind a key can be:

  - ``nil`` (if the data source's ``cb`` returned ``nil``);
  - a string (if the data source's ``cb`` returned a string), or;
  - an *error obect*, which is a function that, when called, returns *data source error specification string*.

    Currently, the following *data source error specification strings* are defined:

    + ``"cb_lua_error"``: a Lua error occurred in a call to data source's ``cb``;
    + ``"plugin_exited"``: data source's plugin reported a fatal error and there will be no more events from this data source.

* ``data_sources``: table

  This must be either an array (table with numeric keys) of *data source specifications*,
  or ``nil`` (which is equivalent to empty array).
  A *data source specification* is a string with a Lua program which defines ``LL_data_source`` global variable.
  This specification is used to initialize a new **nested widget**.
  Just like with normal luastatus widgets, ``plugin`` and ``cb`` entries are required, ``opts`` entry is optional.
  There entries have the same meaning as with normal luastatus widgets.
  Unlike normal luastatus widgets, however, ``event`` entry is ignored for nested widgets.

  Here is an example of a *data source specification*::

    [[
        LL_data_source = {
            plugin = 'xtitle',
            opts = {
                extended_fmt = true,
            },
            cb = function(t)
                return t.instance or ''
            end,
        }
    ]]

* ``greet``: boolean

  Whether to call ``cb`` with ``{what = "hello"}`` in the beginning.

* ``cache_prompt``: boolean

  The value of ``cache_prompt`` JSON parameter to pass in a request to *llama.cpp*.
  If this parameter is true, *llama.cpp* compares the prompt to the previous one
  and only evaluates the unseen suffix.
  Defaults to true.

* ``extra_params_json``: string

  Another parameters to pass in a request to *llama.cpp*, as a JSON encoded to a string.
  The string must be a dictionary content without enclosing ``{}``, for example
  ``"max_tokens":250`` or ``"temperature":0.9,"top_k":10``.

  See `the list of all possible parameters to /completion <https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md#post-completion-given-a-prompt-it-returns-the-predicted-completion>`_.

* ``upd_timeout``: number

  Minimum interval between requests.
  The interval between two successive requests can not be less than this parameter,
  even in the presence of updates.

  If there are no data source, this is the period with which to call the function
  given as the ``prompt`` parameter.

  Defaults to 3.

  Can be set to zero, but only if there is at least one data source (setting this
  to zero with no data sources doesn't make sense and would lead to busy-looping).

* ``tell_about_timeout``: boolean

  If true, time-outs caused by ``upd_timeout`` will be reported to ``cb`` as
  ``{what = "timeout"}``. Defaults to false.

* ``report_mu``: boolean

  Here, "mu" means "more updates". If set to true, after making a request,
  the plugin will check if there are already some pending updates, and
  set ``mu`` field (of the table argument to ``cb``) to a boolean accordingly.

  This can be handy if you want to clear the widget's content if the
  data that has been used used to generate the prompt is already stale/outdated
  after the request is done.

  Defaults to false.

* ``make_self_pipe``: boolean

  If true, the ``wake_up()`` (see the `Functions`_ section) function will be available. Defaults to
  false.

Connection settings
-------------------
The ``connection`` table specifies connections setting to connect to *llama.cpp* REST API.
The following entries are supported:

* ``hostname``: string

  Hostname to connect to. Defaults to ``"127.0.0.1"``.

* ``port``: number

  Port to connect to. Defaults to 8080 (default *llama.cpp* REST API port).

* ``custom_iface``: string

  Custom interface to connect through, in curl format (either an IP address, an interface name,
  or ``"iface!NAME"``/``"host!ADDR"`` to force interpretation of the string as either interface name or address).

* ``use_ssl``: boolean

  Whether or not to use https protocol instead of http. Defaults to false.

* ``req_timeout``: number

  Request timeout in seconds. Defaults to 7. Currently must be an integer;
  if fractional, rounded up to the nearest integer. Zero or negative value
  means no timeout.

* ``log_all_traffic``: boolean

  Whether or not to enable libcurl's debug facilities to log all headers,
  traffic and other information. Defaults to false.

  Note that this may be privacy-related because the request and/or response
  bodies may include window titles and other private data.

* ``log_response_on_error``: boolean

  Whether or not to dump the server response to log on request failure
  (non-2xx HTTP status). Defaults to false.

  Note that this may be privacy-related because the response body may
  include window titles and other private data.

* ``max_response_bytes``: number

  Maximum response size in bytes. The maximum is 4294967295 (4 Gb minus one byte). Defaults to the maximum.

``cb`` argument
===============
The argument is always a table with a ``what`` entry.

* If ``what = "hello"``, the ``greet`` option was set to true and the plugin is currently starting up.

* If ``what = "answer"``, the response from *llama.cpp* has been received; in this case, the table also has a string ``answer`` entry.
  If ``report_mu`` option was set to true, then the table also has a boolean ``mu`` entry.

* If ``what = "fifo"``, the ``make_self_pipe`` option was set to true and this widget issued a call to the ``luastatus.plugin.wake_up()`` function.

* If ``what = "timeout"``, the ``tell_about_timeout`` option was set to true and the plugin has just finished waiting for
  ``upd_timeout`` seconds before getting further updates from the data sources.

* If ``what = "error"``, there was a problem obtaining answer from *llama.cpp* via the REST API.
  In this case, the table also has a string ``error`` entry with human-readable description of
  the problem, and also a string ``meta`` entry, which is intended to be a computer-readable description
  of the problem. See the `Error meta-information`_ section below for more information.

* If ``what = "prompt_error"``, there was a problem generating the prompt: the function passed as the
  ``prompt`` option either threw an error or returned a value of invalid type.
  In this case, you would probably want the barlib to indicate the error by showing an ``(Error)`` segment.
  In order to do this, throw an error out of the ``cb`` function with something like ``error("prompt error!")``.

Error meta-information
----------------------

If the ``cb`` argument has ``what = "error"``, the ``meta`` entry is a string that is indended to be a
comuter-readable descrition of the problem. It has character-and-number format, where the character
signifies the "domain" of the error and number signifies the error code. The error code is an integer,
possibly negative.

The following domains are currently defined:

* ``J``: JSON parsing; error codes are unspecified;

* ``L``: limits: either libcurl's built-in limits on the size of input/output, or the plugin's ``max_response_bytes`` limit; error codes are unspecified;

* ``H``: HTTP status: either HTTP status code is not 2xx (then the error code is the HTTP status), or no proper HTTP response was received (then the error code is 0);

* ``C``: libcurl error; the error code is CURLcode's numeric value (see https://curl.se/libcurl/c/libcurl-errors.html for the list).

An example of error meta-information string is ``"C28"``, which means libcurl error because the request timeout was reached.

Functions
=========

* ``luastatus.plugin.wake_up()``

  Force a call to ``cb`` with ``what="fifo"``. Only available if the ``make_self_pipe`` option was set to true.

* ``luastatus.plugin.push_timeout(timeout)``

  Override the timeout (``upd_timeout`` option) for the next iteration.

* ``luastatus.plugin.push_extra_params_json(str)``

  Override the ``extra_params_json`` option for the next iteration.

* ``escaped_str = luastatus.plugin.escape_double_quoted(str)``

  Escapes a double-quoted string by replacing all double quotes with two single quotes.

* ``escaped_str = luastatus.plugin.escape_single_quoted(str)``

  Escapes a single-quoted string by replacing all single quotes with backquotes.

* ``json = luastatus.plugin.json_escape(str)``

  Escapes a string for JSON encoding.
  Note that this function does not enclose the result in double quotes.

But is it secure?
=================
Some users might be rightfully concerned about the security of using such a plugin:

1. Is the title of the active window (or any other data from nested widgets) transmitted to somewhere over the network?
2. Can this plugin be somehow "hacked" with a specially-crafted window title (or any other data from nested widgets)?
   That is, is it possible that either the title itself, or the LLM's response to it, could cause stack buffer overrun or a similar issue?
   This thing is implemented in C, after all!

The short answer to the questions above is, "no".

The longer answer to the first question is, "no, unless you explicitly configure it to do so".
*llama.cpp* is a tool for running LLMs locally; by default, it only serves for local host, and
this plugin's ``hostname`` defaults to ``"127.0.0.1"``. You *can* run *llama.cpp* on a remote host,
configure it to serve over the network and configure this plugin to perform requests over
the network, but this is not the default. This is the same as with our **mpd** plugin.

The longer answer to the second question is, "no, it's not possible".
Aside from escaping JSON, we don't mess with the strings in any way.
It's all libcurl and YAJL. Even functions provided to Lua (``escape_double_quoted``, ``escape_single_quoted`` and ``json_escape``)
are implemented in Lua instead of C (see ``escape_lfuncs.c``).

This means any strings of the same length are equivalent in terms of security. And there are no
reasons to suspect some string lengths are different from others, for this plugin, in terms of security.
This is mostly due to the fact that we don't use stack allocation for these things; instead, we use
heap-allocated resizable strings.

Just for laughs, we `fuzzed <https://en.wikipedia.org/wiki/Fuzzing>`_ the JSON escaping function (see FUZZING.md),
and `AFL <https://lcamtuf.coredump.cx/afl/>`_ hasn't identified any issues.

Also, luastatus has a comprehensive test suite, which includes many tests for this plugin; it
passes both under **valgrind** and when compiled with **ubsan** (undefined behavior sanitizer,
``-fsanitize=undefined``), under both GCC and Clang.

Users concerned about security of such a setup might also want to verify the security of the underlying
status bar program and intermediate libraries (e.g. YAJL in case of i3bar).
