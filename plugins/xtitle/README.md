This plugin monitors active window title.

Options
===

* `display`: string

  Display to connect to. Default is to use `DISPLAY` environment variable.

* `visible`: boolean

  If true, try to retrieve the title from the `_NET_WM_VISIBLE_NAME` atom. Default is false.

`cb` argument
===
String with title of active window, or `nil` if there is no one.
