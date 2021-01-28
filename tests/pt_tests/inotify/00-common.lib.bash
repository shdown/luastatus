main_fifo_file=./tmp-fifo-main

preface='
local function _fmt_mask(m)
    local ks = {}
    for k, _ in pairs(m) do
        ks[#ks + 1] = k
    end
    table.sort(ks)
    return table.concat(ks, ",")
end
'
