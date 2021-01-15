Writing a widget that uses custom data source
===

A “custom data source” widget is a fancy way to refer to a widget that wants to wait for events
and/or receive data using some blocking Lua operation (most commonly using some external Lua
module), rather than using some luastatus plugin.

The luastatus project explicitly supports this flow; in fact, the “separate state” thing is there
exactly to help such widgets react to events timely.

The basic mechanism is (details may vary for different widgets):

  * Specify `timer` as a plugin, with `period = 0` in plugin options (`opts`).

  * Do the following in the callback function (`cb`):

    + wait for next event (unless this is the first time `cb` is called);

    + generate data for barlib and return it.

  * If need to handle events, use separate-state event handler.

Tricks to know about
---

* Use `luastatus.plugin.push_period(seconds)` function of the `timer` plugin to force it to sleep
for a specified number of seconds after barlib receives your data. Note that this is a
“*push* timeout” operation, not “*set* timeout”, meaning that it will only be respected for a single
iteration, and then “popped” (forgotten).

* Use the argument to `cb` provided by the `timer` plugin to tell if this is the first time `cb`
is called: the argument is a string, and it is `"hello"` if and only if this is the first time.

* The `timer` plugin supports wakeup FIFO. This might be useful if `push_period` is used in case of
error, as a way to force an earlier re-try. The FIFO may be touched from inside the event handler,
or from anywhere else.

List of derived plugins using custom data sources
---

* imap;

* pipe.

List of widget examples using custom data sources
---

* btc-price;

* gmail;

* update-on-click;

* weather.
