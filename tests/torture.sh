#!/usr/bin/env bash
set -e

LUASTATUS=(../luastatus/luastatus ${DEBUG:+-l trace})

VALGRIND=(valgrind --error-exitcode=42)

(
    cd ..
    cmake -DCMAKE_BUILD_TYPE=Debug .
    make -C luastatus
    make -C tests
)

run1() {
    local n=$1 m=$2 sep_st=$3
    if (( sep_st )); then
        local event_beg='function()' event_end='end'
    else
        local event_beg='[['         event_end=']]'
    fi
    shift 3
    "${VALGRIND[@]}" "$@" "${LUASTATUS[@]}" -e -b ./mock_barlib.so -B gen_events="$m" <(cat <<__EOF__
n = 0
widget = {
    plugin = './mock_plugin.so',
    opts = {
        make_calls = $n,
    },
    cb = function()
        n = n + 1
        if n % 10000 == 0 then
            --print('--- n = ' .. n .. ' ---')
        end
    end,
    event = $event_beg
        m = (m or 0) + 1
        if m % 10000 == 0 then
            --print('--- m = ' .. m .. ' ---')
        end
    $event_end,
}
__EOF__
)
}

run2() {
    run1 "$1" "$2" 0 "${@:3}" || return $?
    run1 "$1" "$2" 1 "${@:3}" || return $?
}

run2 10000 10000 \
    --suppressions=dlopen.supp \
    --leak-check=full \
    --show-leak-kinds=all \
    --errors-for-leak-kinds=all \
    --track-fds=yes
# Interesting options:
#     --fair-sched=yes

run2 100000 100000 \
    --tool=helgrind

echo >&2 "=== PASSED ==="
