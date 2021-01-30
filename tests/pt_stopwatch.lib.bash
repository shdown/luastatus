coproc "$PT_BUILD_DIR"/tests/stopwatch "${PT_MAX_LAG:-75}" \
    || pt_fail "coproc failed"

STOPWATCH_PID=$COPROC_PID
STOPWATCH_FDS=( "${COPROC[0]}" "${COPROC[1]}" )

measure_start() {
    echo s >&${STOPWATCH_FDS[1]} || pt_fail "Cannot communicate with stopwatch."
}

measure_get_ms() {
    echo q >&${STOPWATCH_FDS[1]} || pt_fail "Cannot communicate with stopwatch."
    local ans
    IFS= read -r ans <&${STOPWATCH_FDS[0]} || pt_fail "Cannot communicate with stopwatch."
    echo "$ans"
}

measure_check_ms() {
    echo "c $1" >&${STOPWATCH_FDS[1]}
    local ans
    IFS= read -r ans <&${STOPWATCH_FDS[0]} || pt_fail "Cannot communicate with stopwatch."
    if [[ $ans != 1* ]]; then
        pt_fail "measure_check_ms $1: stopwatch said: '$ans'."
    fi
}
