This derived plugin monitors the number of unread mails in an IMAP mailbox.

Functions
===
  - `widget(tbl)`

      Constructs a `widget` table required by luastatus. `tbl` is a table with
      the following fields:

      **(required)**

      * `cb`: a callback function that will be called with the number of unread mails, or with `nil` if it is unknown;

      * `host`: host string;

      * `port`: port number;

      * `login`, `password`: your credentials (strings);

      * `mailbox`: mailbox name (string; typically `"Inbox"`);

      * `error_sleep_period`: a number of seconds to sleep after an error.

      **(optional)**

      * `use_ssl`: boolean specifying whether to use SSL (you probably should set it to `true`). Defaults to false;

      * `verbose`: boolean specifying whether to be verbose (useful for troubleshooting). Default to false;

      * `timeout`: idle timeout, in seconds, after which to reconnect; you probably want to set this to something like `5 * 60`;

      * `handshake_timeout`: SSL handshake timeout, in seconds;

      * `event`: event entry of the resulting table.
