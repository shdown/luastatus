tmpdir=$(mktemp -d) || pt_fail 'mktemp -d failed'
pt_add_file_to_remove "$tmpdir"/.hidden
pt_add_file_to_remove "$tmpdir"/normal
pt_add_dir_to_remove "$tmpdir"

x_my_callback_dirnonempty() {
    case "$1" in
    0)
        ;;
    1)
        pt_check touch "$tmpdir"/.hidden
        ;;
    2)
        pt_check touch "$tmpdir"/normal
        ;;
    3)
        pt_check rm -- "$tmpdir"/normal
        ;;
    esac
}

is_program_running_testcase \
    "kind='dir_nonempty', path='$tmpdir'" \
    x_my_callback_dirnonempty \
    'cb false' \
    'cb false' \
    'cb true' \
    'cb false'
