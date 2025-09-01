network_rate_linux_testcase_iface_filter() {
    local lua_opts=$1
    shift

    local expected_out=

    local letter
    for letter in "$@"; do
        case "$letter" in
            L) expected_out+='lo=>{R:0.0,S:0.0},' ;;
            T) expected_out+='tun0=>{R:0.0,S:0.0},' ;;
            W) expected_out+="wlan0=>{$res_wlan0_div1}," ;;
            *) pt_fail "unexpected letter '$letter'" ;;
        esac
    done

    expected_out=${expected_out%,}

    network_rate_linux_testcase "$lua_opts" "cb [$expected_out]"
}

network_rate_linux_testcase_iface_filter 'iface_only="wlan0"' W
network_rate_linux_testcase_iface_filter 'iface_except="lo"' T W

network_rate_linux_testcase_iface_filter 'iface_only={"tun0","wlan0"}' T W
network_rate_linux_testcase_iface_filter 'iface_except={"lo","tun0"}' W

network_rate_linux_testcase_iface_filter 'iface_only={tun0=true,wlan0=true}' T W
network_rate_linux_testcase_iface_filter 'iface_except={lo=true,tun0=true}' W

network_rate_linux_testcase_iface_filter 'iface_filter=function(s) return string.find(s, "w") == nil end' L T
