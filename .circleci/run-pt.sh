#!/usr/bin/env bash

set -e

./.circleci/mini-systemd.sh init
./.circleci/mini-systemd.sh launch pa pulseaudio --daemonize=no

c=0
PT_TOOL=valgrind PT_MAX_LAG=200 ./tests/pt.sh . skip:plugin-dbus || c=$?

./.circleci/mini-systemd.sh kill-everything

exit "$c"
