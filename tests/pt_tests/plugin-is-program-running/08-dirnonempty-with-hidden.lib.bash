tmpdir=$(mktemp -d) || pt_fail 'mktemp -d failed'
pt_add_file_to_remove "$tmpdir"/.hidden
pt_add_file_to_remove "$tmpdir"/normal
pt_add_dir_to_remove "$tmpdir"

x_my_callback_dirnonempty_with_hidden() {
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
    4)
        pt_check rm -- "$tmpdir"/.hidden
        ;;
    esac
    echo "=== After $i: ===" >&2
    find "$tmpdir" >&2
    echo "=================" >&2
}

is_program_running_testcase \
    "kind='dir_nonempty_with_hidden', path='$tmpdir'" \
    x_my_callback_dirnonempty_with_hidden \
    'cb false' \
    'cb true' \
    'cb true' \
    'cb true' \
    'cb false'
