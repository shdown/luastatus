#!/bin/sh

if [ "$#" -ne 2 ]; then
    echo >&2 "USAGE: $0 INPUT OUTPUT"
    exit 2
fi

# 1. Escape backslashes (\ -> \\) and double quotes (" -> \").
# 2. Replace tabs with \t.
# 3. Try to handle C trigraphs.
# 4. Insert \n at the end, enclose the whole line in double quotes.
sed -r 's/[\\"]/\\\0/g; s/\t/\\t/g; s/\?/""\0""/g; s/.*/"\0\\n"/' -- "$1" > "$2"
