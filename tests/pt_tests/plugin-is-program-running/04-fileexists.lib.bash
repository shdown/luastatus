is_program_running_testcase \
    'kind="file_exists", path="/"' \
    x_do_nothing \
    'cb true'

tmpfile=$(mktemp) || pt_fail 'mktemp failed'
pt_add_file_to_remove "$tmpfile"

x_my_callback_del_tmpfile() {
    if (( $1 == 1 )); then
        pt_check rm -- "$tmpfile"
    fi
}

is_program_running_testcase \
    "kind='file_exists', path='$tmpfile'" \
    x_my_callback_del_tmpfile \
    'cb true' \
    'cb false' \
