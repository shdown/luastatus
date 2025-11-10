Implementation of luastatus.libwidechar module.
It contains functions working on wchar_t strings to:
  1. Determine the width of a string;
  2. Truncate a string to a given width;
  3. Convert a byte string into a valid string of printable wchar_t's,
     replacing illegal sequences or unprintable characters with a given
     placeholder.
