#!/usr/bin/env bash

set -e

mytempfile=/tmp/mini-systemd-data.txt

usage() {
    echo >&2 "USAGE: $0 {init | launch KEYWORD COMMAND [ARGS...] | kill-everything}"
    exit 2
}

if (( $# < 1 )); then
    usage
fi
command=$1; shift
case "$command" in
init)
    if (( $# > 0 )); then
        usage
    fi
    true > "$mytempfile"
    echo >&2 "[mini-systemd] Initialized successfully."
    ;;
launch)
    if (( $# < 2 )); then
        usage
    fi
    keyword=$1; shift
    "$@" &
    pid=$!
    echo "$keyword $pid" >> "$mytempfile"
    echo >&2 "[mini-systemd] Launched '$keyword' (PID $pid)."
    ;;
kill-everything)
    if (( $# > 0 )); then
        usage
    fi
    while read keyword pid; do
        echo >&2 "[mini-systemd] Killing '$keyword' (PID $pid)..."
        kill "$pid" || true
    done < "$mytempfile"
    ;;
*)
    usage
    ;;
esac
