main_fifo_file=./tmp-fifo-main
proc_dir=$(mktemp -d) || fail "Cannot create temporary directory."
meminfo_file=$proc_dir/meminfo

mem_usage_testcase() {
    local testcase_name=$1
    local expect_str=$2
    local meminfo_content=$3

    testcase_begin "mem-usage-linux/$testcase_name"
    add_fifo "$main_fifo_file"
    printf '%s' "$meminfo_content" > "$meminfo_file" || fail "Cannot write to $meminfo_file."
    add_file_to_remove "$meminfo_file"
    write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$SOURCE_DIR/plugins/mem-usage-linux/mem-usage-linux.lua')
widget = x.widget{
    _procpath = '$proc_dir',
    cb = function(t)
        f:write(string.format('cb avail=%s(%s) total=%s(%s)\n', t.avail.value, t.avail.unit, t.total.value, t.total.unit))
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

mem_usage_testcase 'simple' 'cb avail=1527556(kB) total=3877576(kB)' "\
MemTotal:        3877576 kB
MemFree:          774760 kB
MemAvailable:    1527556 kB
Buffers:          178504 kB
Cached:          1263372 kB
SwapCached:            0 kB
Active:           458264 kB
Inactive:        1938980 kB
Active(anon):       5360 kB
Inactive(anon):  1489156 kB
Active(file):     452904 kB
Inactive(file):   449824 kB
Unevictable:      394212 kB
Mlocked:             144 kB
SwapTotal:             0 kB
SwapFree:              0 kB
Dirty:               108 kB
Writeback:             0 kB
AnonPages:       1259188 kB
Mapped:           336640 kB
Shmem:            539148 kB
KReclaimable:     115248 kB
Slab:             212356 kB
SReclaimable:     115248 kB
SUnreclaim:        97108 kB
KernelStack:        6976 kB
PageTables:        26212 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:     1938788 kB
Committed_AS:    6041840 kB
VmallocTotal:   34359738367 kB
VmallocUsed:       27600 kB
VmallocChunk:          0 kB
Percpu:             2208 kB
HardwareCorrupted:     0 kB
AnonHugePages:    444416 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
FileHugePages:         0 kB
FilePmdMapped:         0 kB
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:               0 kB
DirectMap4k:      836220 kB
DirectMap2M:     3203072 kB
DirectMap1G:           0 kB
"

rmdir "$proc_dir" || fail "Cannot rmdir $proc_dir."
