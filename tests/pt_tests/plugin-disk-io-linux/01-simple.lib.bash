content1="\
 259       0 nvme0n1 137275 50367 21312112 79036 259141 59459 47106050 879950 0 101996 979461 0 0 0 0 5700 20474
 259       1 nvme0n1p1 187 0 7679 88 10 0 10 17 0 100 106 0 0 0 0 0 0
 259       2 nvme0n1p2 25027 8539 2421066 15822 3282 1336 210032 22593 0 17612 38416 0 0 0 0 0 0
 259       3 nvme0n1p3 111967 41828 18880887 63111 255847 58123 46896008 857339 43 90680 920450 0 0 0 0 0 0
 254       0 dm-0 153758 0 18880082 102332 313970 0 46896008 5605212 44 95076 5707544 0 0 0 0 0 0
"

content2="\
 259       0 nvme0n1 141523 50367 22387160 81628 263894 59459 48322570 900083 0 102748 1002186 0 0 0 0 5700 20474
 259       1 nvme0n1p1 187 0 7679 88 10 0 10 17 0 100 106 0 0 0 0 0 0
 259       2 nvme0n1p2 25027 8539 2421066 15822 3282 1336 210032 22593 0 17612 38416 0 0 0 0 0 0
 259       3 nvme0n1p3 116215 41828 19955935 65702 260600 58123 48112528 877472 74 91556 943175 0 0 0 0 0 0
 254       0 dm-0 158006 0 19955130 105076 318723 0 48112528 5662204 75 96024 5767280 0 0 0 0 0 0
"

awk_program='
BEGIN {
    SECTOR_SIZE = 512
}
$0 != "" {
    disk_id = $1 "," $2 " " $3
    r = $6
    w = $10
    if (!seen[disk_id]) {
        seen[disk_id] = 1
        seen_r[disk_id] = r
        seen_w[disk_id] = w
    } else {
        delta_r = r - seen_r[disk_id]
        delta_w = w - seen_w[disk_id]
        print(disk_id, delta_r * SECTOR_SIZE, delta_w * SECTOR_SIZE)
    }
}'

expect_str=

gen_expect_str() {
    expect_str='cb'

    local maj_min
    local name
    local delta_r
    local delta_w
    while read maj_min name delta_r delta_w; do
        expect_str+=" (${maj_min} name=>${name} r=>${delta_r} w=>${delta_w})"
    done < <(
        printf '%s\n%s\n' "${content1}${content2}" \
            | awk "$awk_program" \
            | LC_ALL=C sort -k2,2
    )
}

gen_expect_str

disk_io_linux_testcase 'cb <none>' "$expect_str" "$content1" "$content2"
