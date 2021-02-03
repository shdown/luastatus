pt_testcase_begin
pt_add_fifo "$main_fifo_file"

unset_env_var=CzRbFEXexi
while [[ -v $unset_env_var ]]; do
    unset_env_var+='a'
done

pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
function dump(varname)
    local r = os.getenv(varname)
    if r then
        f:write(string.format('%s: %s\n', varname, r))
    else
        f:write(string.format('%s is not set\n', varname))
    end
end
dump('MY_ENV_VAR')
dump('$unset_env_var')
__EOF__

MY_ENV_VAR='<>?foo  bar~~~' pt_spawn_luastatus_directly -b "$mock_barlib"

exec {pfd}<"$main_fifo_file"
pt_expect_line 'MY_ENV_VAR: <>?foo  bar~~~' <&$pfd
pt_expect_line "$unset_env_var is not set" <&$pfd
pt_close_fd "$pfd"

pt_testcase_end
