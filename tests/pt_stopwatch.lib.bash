using_measure() {
    pt_spawn_thing_pipe stopwatch "$PT_BUILD_DIR"/tests/stopwatch "${PT_MAX_LAG:-75}"
}

measure_start() {
    if ! pt_has_spawned_thing stopwatch; then
        pt_fail_internal_error "measure_start: stopwatch was not spawned (did you forget 'using_measure'?)."
    fi
    echo s >&${PT_SPAWNED_THINGS_FDS_1[stopwatch]}
}

measure_get_ms() {
    echo q >&${PT_SPAWNED_THINGS_FDS_1[stopwatch]}
    local ans
    IFS= read -r ans <&${PT_SPAWNED_THINGS_FDS_0[stopwatch]} \
        || pt_fail "Cannot read next line from stopwatch (the process died?)."
    printf '%s\n' "$ans"
}

measure_check_ms() {
    echo "c $1" >&${PT_SPAWNED_THINGS_FDS_1[stopwatch]}
    local ans
    IFS= read -r ans <&${PT_SPAWNED_THINGS_FDS_0[stopwatch]} \
        || pt_fail "Cannot read next line from stopwatch (the process died?)."
    if [[ $ans != 1* ]]; then
        pt_fail "measure_check_ms $1: stopwatch said: '$ans'."
    fi
}
