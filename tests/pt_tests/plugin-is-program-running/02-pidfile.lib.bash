pidfile=$(mktemp) || pt_fail 'mktemp failed'
pt_add_file_to_remove "$pidfile"

write_pidfile() {
    printf '%s\n' "$1" > "$pidfile" || pt_fail 'cannot write $pidfile'
}

x_my_callback() {
    case "$1" in
    0)
        write_pidfile junk > "$pidfile"
        ;;
    1)
        write_pidfile "$$" > "$pidfile"
        ;;
    2)
        write_pidfile 0 > "$pidfile"
        ;;
    3)
        write_pidfile 1 > "$pidfile"
        ;;
    4)
        write_pidfile -1 > "$pidfile"
        ;;
    5)
        true > "$pidfile" || pt_fail 'cannot write $pidfile'
        ;;
    esac
}

is_program_running_testcase \
    "kind='pidfile', path='$pidfile'" \
    x_my_callback \
    'cb false' \
    'cb true' \
    'cb false' \
    'cb true' \
    'cb false' \
    'cb false'
