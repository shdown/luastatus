This plugin monitors state of an mpd server.

Options
===
* `hostname`: string

  Hostname to connect to. Default is to connect to the local host.

* `port`: number

  Port to connect to. Default is 6600.

* `password`: string

  Server’s password.

* `timeout`: number

  Number of seconds to wait before calling `cb` with `what="timeout"` (after a connection has been established). May be fractional. If not specified or negative, `cb` is never called with `what="timeout"`.

* `retry_in`: number

  Number of seconds to retry in after the connection is lost. A negative value means do not retry and return immediately. May be fractional. Defaults to 10.

* `retry_fifo`: string

  Path to an existent FIFO. The plugin does not create FIFO itself. To force a reconnect, `touch(1)` the FIFO, that is, open it for writing and then close.

`cb` argument
===
A table with `what` entry.

  * If it’s `"connecting"`, the plugin is now connecting to the server.

  * If it’s `"update"`, either the plugin has just connected to the server and queried its state, or the server has just changed its state. Additionally, the following entries are provided:
    * `song`: table with server’s response to the `currentsong` command. Interestingly, it is not documented at all, so here is an example:
      - `file`: `08. Sensou to Heiwa.mp3`
      - `Last-Modified`: `2016-07-31T09:56:31Z`
      - `Artist`: `ALI PROJECT`
      - `AlbumArtist`: `ALI PROJECT`
      - `Title`: `戦争と平和`
      - `Album`: `Erotic & Heretic`
      - `Track`: `8`
      - `Date`: `2002-02-20`
      - `Genre`: `J-Pop`
      - `Disc`: `1/1`
      - `Time`: `260`
      - `Pos`: `0`
      - `Id`: `4`

      All values are strings, use `tonumber` if needed.

    * `status`: table with server’s response to the `status` command. See the `status` command description [there](https://www.musicpd.org/doc/protocol/command_reference.html). All values are strings, use `tonumber` if needed.

  * It it’s `"timeout"`, the server hasn’t changed its state for the number of seconds specified as the `timeout` option.

  * If it’s `"error"`, the connection has been lost. The plugin is now going to sleep and try to reconnect, or to return.
