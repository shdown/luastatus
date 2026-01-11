temperature_linux_testcase "$common_carcass" \
    'function(kind, name) return kind == "thermal" end' \
    '^thermal:.*$'
