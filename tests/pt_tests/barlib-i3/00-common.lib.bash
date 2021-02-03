main_fifo_file=./tmp-fifo-main

x_spawn_luastatus() {
    pt_spawn_luastatus_for_barlib_test_via_runner \
        "$PT_SOURCE_DIR"/tests/barlib-runners/runner-redirect-34 \
        -b "$PT_BUILD_DIR"/barlibs/i3/barlib-i3.so -B in_fd=3 -B out_fd=4 \
        "$@"
}
