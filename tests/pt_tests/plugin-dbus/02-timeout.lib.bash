pt_require_tools dbus-send

pt_testcase_begin
using_measure

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/dbus/plugin-dbus.so',
    opts = {
        signals = {
            {
                object_path = '/org/luastatus/sample/object/name',
                interface = 'org.luastatus.ExampleInterface',
                bus = 'session',
            },
        },
        timeout = 2,
        greet = true,
    },
    cb = function(t)
        if t.what == 'signal' then
            assert(type(t.sender) == 'string')
            t.sender = '(non-deterministic)'
        end
        f:write('cb ' .. _fmt_x(t) .. '\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'cb {["what"]="hello"}' <&$pfd

# Try to avoid race condition...
pt_expect_line 'cb {["what"]="timeout"}' <&$pfd

for (( i = 0; i < 3; ++i )); do
    dbus-send \
        --session \
        /org/luastatus/sample/object/name \
        org.luastatus.ExampleInterface.ExampleMethod \
        int32:47 string:'hello world' double:63.125 \
        array:string:"1st item","next item","last item" \
        dict:string:int32:"one",1,"two",2,"three",3 \
        variant:int32:-8 \
        objpath:/org/luastatus/sample/object/name

    measure_start
    pt_expect_line 'cb {["bus"]="session",["interface"]="org.luastatus.ExampleInterface",["object_path"]="/org/luastatus/sample/object/name",["parameters"]={"47","hello world",63.1250,{"1st item","next item","last item"},{{"one","1"},{"two","2"},{"three","3"}},"-8","/org/luastatus/sample/object/name"},["sender"]="(non-deterministic)",["signal"]="ExampleMethod",["what"]="signal"}' <&$pfd
    measure_check_ms 0

    pt_expect_line 'cb {["what"]="timeout"}' <&$pfd
    measure_check_ms 2000

    pt_expect_line 'cb {["what"]="timeout"}' <&$pfd
    measure_check_ms 2000
done

pt_close_fd "$pfd"
pt_testcase_end
