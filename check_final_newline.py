#!/usr/bin/env python3
import os
import sys


def say(s: str) -> None:
    print(s, file=sys.stderr)


def main() -> None:
    args = sys.argv[1:]
    if not args:
        say('USAGE: check_final_newline.py FILE [FILE ...]')
        sys.exit(2)

    everything_ok = True
    for arg in args:
        with open(arg, 'rb') as f:
            f.seek(0, os.SEEK_END)
            file_size = f.tell()
            if not file_size:
                continue
            f.seek(-1, os.SEEK_END)
            final_byte = f.read(1)
            if final_byte != b'\n':
                say(f'File "{arg}" has no trailing newline!')
                everything_ok = False

    if everything_ok:
        sys.exit(0)
    else:
        sys.exit(1)


if __name__ == '__main__':
    main()
