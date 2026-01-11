temperature_linux_testcase "$common_carcass" \
    'function(kind, name) return (name:find("t")) ~= nil end' \
    '^.*:.*t.*$'
