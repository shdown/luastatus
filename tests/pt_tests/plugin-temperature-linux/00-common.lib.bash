pt_require_tools mktemp

main_fifo_file=./tmp-fifo-main

temperature_linux_add_dir() {
    local path=$1

    mkdir -- "$path" || pt_fail "cannot create directory '$path'"
    pt_add_dir_to_remove "$path"
}

temperature_linux_add_file() {
    local path=$1
    local content=$2

    printf '%s\n' "$content" > "$path" || pt_fail "cannot write file '$path'"
    pt_add_file_to_remove "$path"
}

temperature_linux_create_carcass() {
    local tmp_root=$1
    local carcass=$2

    mkdir "$tmp_root"/hwmon || pt_fail "cannot create hwmon dir ($tmp_root/hwmon)"
    mkdir "$tmp_root"/thermal || pt_fail "cannot create thermal dir ($tmp_root/thermal)"

    local ftype
    local path
    local content
    local dont_care_1
    local dont_care_2
    while read ftype path content dont_care_1 dont_care_2; do
        if [[ $ftype == D ]]; then
            temperature_linux_add_dir "$tmp_root"/"$path"
        elif [[ $ftype == F ]]; then
            temperature_linux_add_file "$tmp_root"/"$path" "$content"
        else
            pt_fail "Unexpected ftype '$ftype'"
        fi
    done < <(printf '%s\n' "$carcass" | sed '/^$/d')

    pt_add_dir_to_remove "$tmp_root"/hwmon
    pt_add_dir_to_remove "$tmp_root"/thermal
}

temperature_linux_make_awk_program() {
    local filter_regex=$1
    cat <<EOF
{
    ftype = \$1
    path = \$2
    content = \$3
    kind = \$4
    name = \$5
    if (ftype != "F") { next }
    if (kind == "_") { next }

    ident = kind ":" name
    if (!(ident ~ /$filter_regex/)) { next }

    print path, content, kind, name
}
EOF
}

temperature_linux_make_expect_str() {
    local carcass=$1
    local filter_regex=$2

    local awk_program=$(temperature_linux_make_awk_program "$filter_regex")

    local res='cb'

    local path
    local content
    local kind
    local name
    while read path content kind name; do
        res+=" (kind=>$kind, name=>$name, value=>$content)"
    done < <(
        printf '%s' "$carcass" \
            | awk "$awk_program" \
            | LC_ALL=C sort -k1,1
    )

    printf '%s\n' "$res"
}

temperature_linux_testcase() {
    local carcass=$1
    local filter_func=${2:-nil}
    local filter_regex=${3:-'^.*$'}

    local expect_str=$(temperature_linux_make_expect_str "$carcass" "$filter_regex") \
        || pt_fail 'temperature_linux_make_expect_str failed'

    pt_testcase_begin
    pt_add_fifo "$main_fifo_file"
    local tmp_root; tmp_root=$(mktemp -d) || pt_fail "'mktemp -d' failed"

    temperature_linux_create_carcass "$tmp_root" "$carcass" \
        || pt_fail 'temperature_linux_create_carcass failed'

    pt_add_dir_to_remove "$tmp_root"

    pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$PT_SOURCE_DIR/plugins/temperature-linux/temperature-linux.lua')
_data = {
    _thermal_path = '$tmp_root/thermal',
    _hwmon_path = '$tmp_root/hwmon',
    filter_func = $filter_func,
}
_opts = {
    cb = function(t)
        if not t then
            f:write('cb nil\n')
        end

        f:write('cb')

        table.sort(t, function(a, b)
            return a.path < b.path
        end)

        for _, x in ipairs(t) do
            f:write(string.format(
                " (kind=>%s, name=>%s, value=>%.0f)",
                x.kind,
                x.name,
                x.value * 1000
            ))
        end

        f:write('\n')
    end,
    event = my_event_func,
}
widget = x.widget(_opts, _data)
widget.plugin = ('$PT_BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__

    pt_spawn_luastatus
    exec {pfd}<"$main_fifo_file"
    pt_expect_line 'init' <&$pfd

    pt_expect_line "$expect_str" <&$pfd

    pt_close_fd "$pfd"
    pt_testcase_end
}
