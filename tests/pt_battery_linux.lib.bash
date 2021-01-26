main_fifo_file=./tmp-fifo-main
batdev_dir=$(mktemp -d) || fail "Cannot create temporary directory."
uevent_file=$batdev_dir/uevent

battery_testcase() {
    local testcase_name=$1
    local expect_str=$2
    local uevent_content=$3 # empty value here means remove the file

    testcase_begin "battery-linux/$testcase_name"
    add_fifo "$main_fifo_file"
    if [[ -n "$uevent_content" ]]; then
        printf '%s' "$uevent_content" > "$uevent_file" || fail "Cannot write to $uevent_file."
    else
        rm -f "$uevent_file" || fail "Cannot remove $uevent_file."
    fi
    add_file_to_remove "$uevent_file"
    write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$SOURCE_DIR/plugins/battery-linux/battery-linux.lua')
widget = x.widget{
    _devpath = '$batdev_dir',
    cb = function(t)
        f:write('cb')
        if t.status then
            f:write(string.format(' status="%s"', t.status))
        end
        if t.capacity then
            f:write(string.format(' capacity=%.0f', t.capacity))
        end
        if t.consumption then
            f:write(string.format(' consumption=%.1f', t.consumption))
        end
        if t.rem_time then
            f:write(string.format(' rem_time=%.1f', t.rem_time))
        end
        f:write('\n')
    end,
    event = my_event_func,
}
widget.plugin = ('$BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__
    spawn_luastatus
    exec 3<"$main_fifo_file"
    expect_line 'init' <&3
    expect_line "$expect_str" <&3
    exec 3<&-
    testcase_end
}

battery_testcase 'full_status_full' 'cb status="Full" capacity=100' "\
POWER_SUPPLY_NAME=BAT0
POWER_SUPPLY_TYPE=Battery
POWER_SUPPLY_STATUS=Full
POWER_SUPPLY_PRESENT=1
POWER_SUPPLY_TECHNOLOGY=Li-ion
POWER_SUPPLY_CYCLE_COUNT=0
POWER_SUPPLY_VOLTAGE_MIN_DESIGN=15200000
POWER_SUPPLY_VOLTAGE_NOW=17074000
POWER_SUPPLY_CURRENT_NOW=0
POWER_SUPPLY_CHARGE_FULL_DESIGN=3030000
POWER_SUPPLY_CHARGE_FULL=3289000
POWER_SUPPLY_CHARGE_NOW=3285000
POWER_SUPPLY_CAPACITY=99
POWER_SUPPLY_CAPACITY_LEVEL=Normal
POWER_SUPPLY_MODEL_NAME=R14B01W
POWER_SUPPLY_MANUFACTURER=Coslight
POWER_SUPPLY_SERIAL_NUMBER=33468
"

battery_testcase 'full_status_not_charging' 'cb status="Not charging" capacity=100' "\
POWER_SUPPLY_NAME=BAT0
POWER_SUPPLY_TYPE=Battery
POWER_SUPPLY_STATUS=Not charging
POWER_SUPPLY_PRESENT=1
POWER_SUPPLY_TECHNOLOGY=Li-ion
POWER_SUPPLY_CYCLE_COUNT=0
POWER_SUPPLY_VOLTAGE_MIN_DESIGN=15200000
POWER_SUPPLY_VOLTAGE_NOW=17074000
POWER_SUPPLY_CURRENT_NOW=0
POWER_SUPPLY_CHARGE_FULL_DESIGN=3030000
POWER_SUPPLY_CHARGE_FULL=3289000
POWER_SUPPLY_CHARGE_NOW=3285000
POWER_SUPPLY_CAPACITY=99
POWER_SUPPLY_CAPACITY_LEVEL=Normal
POWER_SUPPLY_MODEL_NAME=R14B01W
POWER_SUPPLY_MANUFACTURER=Coslight
POWER_SUPPLY_SERIAL_NUMBER=33468
"

battery_testcase 'discharging' 'cb status="Discharging" capacity=99 consumption=18.6 rem_time=2.9' "\
POWER_SUPPLY_NAME=BAT0
POWER_SUPPLY_TYPE=Battery
POWER_SUPPLY_STATUS=Discharging
POWER_SUPPLY_PRESENT=1
POWER_SUPPLY_TECHNOLOGY=Li-ion
POWER_SUPPLY_CYCLE_COUNT=0
POWER_SUPPLY_VOLTAGE_MIN_DESIGN=15200000
POWER_SUPPLY_VOLTAGE_NOW=16806000
POWER_SUPPLY_CURRENT_NOW=1107000
POWER_SUPPLY_CHARGE_FULL_DESIGN=3030000
POWER_SUPPLY_CHARGE_FULL=3285000
POWER_SUPPLY_CHARGE_NOW=3247000
POWER_SUPPLY_CAPACITY=98
POWER_SUPPLY_CAPACITY_LEVEL=Normal
POWER_SUPPLY_MODEL_NAME=R14B01W
POWER_SUPPLY_MANUFACTURER=Coslight
POWER_SUPPLY_SERIAL_NUMBER=33468
"

battery_testcase 'charging' 'cb status="Charging" capacity=94 consumption=12.7 rem_time=0.3' "\
POWER_SUPPLY_NAME=BAT0
POWER_SUPPLY_TYPE=Battery
POWER_SUPPLY_STATUS=Charging
POWER_SUPPLY_PRESENT=1
POWER_SUPPLY_TECHNOLOGY=Li-ion
POWER_SUPPLY_CYCLE_COUNT=0
POWER_SUPPLY_VOLTAGE_MIN_DESIGN=15200000
POWER_SUPPLY_VOLTAGE_NOW=17131000
POWER_SUPPLY_CURRENT_NOW=743000
POWER_SUPPLY_CHARGE_FULL_DESIGN=3030000
POWER_SUPPLY_CHARGE_FULL=3298000
POWER_SUPPLY_CHARGE_NOW=3096000
POWER_SUPPLY_CAPACITY=93
POWER_SUPPLY_CAPACITY_LEVEL=Normal
POWER_SUPPLY_MODEL_NAME=R14B01W
POWER_SUPPLY_MANUFACTURER=Coslight
POWER_SUPPLY_SERIAL_NUMBER=33468
"

battery_testcase 'nofile' 'cb' ''

rmdir "$batdev_dir" || fail "Cannot rmdir $batdev_dir."
