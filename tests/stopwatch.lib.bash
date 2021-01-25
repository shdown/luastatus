MEASURE_LAST=
MEASURE_MAX_LAG_MS=${MAX_LAG:-75}

stopwatch_now() {
    date +%s.%N
}

stopwatch_delta_ms() {
    awk "BEGIN { printf(\"%d\\n\", ($1 - $2) * 1000) }"
}

measure_start() {
    MEASURE_LAST=$(stopwatch_now) || fail "stopwatch_now failed"
}

measure_get_ms() {
    if [[ -z $MEASURE_LAST ]]; then
        fail_internal_error "measure_get_ms: MEASURE_LAST is empty"
    fi

    local now
    now=$(stopwatch_now) || fail "stopwatch_now failed"

    local delta
    delta=$(stopwatch_delta_ms "$now" "$MEASURE_LAST") || fail "stopwatch_delta_ms failed"

    MEASURE_LAST=$now

    printf '%s\n' "$delta"
}

measure_check_ms() {
    if [[ -z $MEASURE_LAST ]]; then
        fail_internal_error "measure_get_ms: MEASURE_LAST is empty"
    fi

    local now
    now=$(stopwatch_now) || fail "stopwatch_now failed"

    local delta
    delta=$(stopwatch_delta_ms "$now" "$MEASURE_LAST") || fail "stopwatch_delta_ms failed"
    if (( delta < $1 - MEASURE_MAX_LAG_MS )); then
        fail "Actual delay ($delta ms) is not $1 (+/- $MEASURE_MAX_LAG_MS) ms (less)."
    fi
    if (( delta > $1 + MEASURE_MAX_LAG_MS )); then
        fail "Actual delay ($delta ms) is not $1 (+/- $MEASURE_MAX_LAG_MS) ms (greater)."
    fi

    MEASURE_LAST=$now
}
