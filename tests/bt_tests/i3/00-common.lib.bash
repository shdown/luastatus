main_fifo_file=./tmp-fifo-main

x_spawn_luastatus() {
    bt_spawn_luastatus_via_runner "$BT_SOURCE_DIR"/tests/bt_runners/runner-redirect-34 \
        -b "$BT_BUILD_DIR"/barlibs/i3/barlib-i3.so -B in_fd=3 -B out_fd=4 \
        "$@"
}
