#!/usr/bin/env bash

set -e

sudo ./.circleci/mini-systemd.sh init
sudo ./.circleci/mini-systemd.sh launch pa pulseaudio --daemonize=no

PT_TOOL=valgrind PT_MAX_LAG=200 ./tests/pt.sh . skip:plugin-dbus

sudo ./.circleci/mini-systemd.sh kill-everything
