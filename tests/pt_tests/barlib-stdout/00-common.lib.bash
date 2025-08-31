main_fifo_file=./tmp-fifo-main
in_fifo_file=./tmp-fifo-main-ev-input

x_spawn_luastatus() {
    pt_spawn_luastatus_for_barlib_test_via_runner \
        "$PT_SOURCE_DIR"/tests/barlib-runners/runner-redirect-34 \
        -b "$PT_BUILD_DIR"/barlibs/stdout/barlib-stdout.so -B out_fd=4 \
        "$@"
}
