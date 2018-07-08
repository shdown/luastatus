This derived plugin monitors the content of a file.

Functions
===

  - `widget(tbl)`

        Constructs a `widget` table required by luastatus. `tbl` is a table with
        the following fields:

        **(required)**

        * `filename`: path to the file to monitor;

        * `cb`: a callback function that will be called with `filename` opened for reading;

        **(optional)**

        * `timeout`, `flags`: better do not touch these — or see the code;

        * `event`: event entry of the resulting table.
