local function load_fortunes()
    local f = assert(io.popen([[
        src=/usr/share/games/fortunes/bofh-excuses
        grep -vE '^(BOFH excuse #[0-9]+:|\s*|%)$' < "$src"
    ]], 'r'))
    local res = {}
    for line in f:lines() do
        table.insert(res, line)
    end
    f:close()
    return res
end
local FORTUNES = load_fortunes()
assert(#FORTUNES ~= 0)

local PROMPT_TEMPLATE = [[<start_of_turn>user
You are a helpful assistant

Hello<end_of_turn>
<start_of_turn>model
Hi there<end_of_turn>
<start_of_turn>user
@@@<end_of_turn>
<start_of_turn>model
]]

widget = {
    plugin = 'llamacxx',
    opts = {
        prompt = function()
            local fortune = FORTUNES[math.random(1, #FORTUNES)]
            -- string.gsub returns two values: result and number of replacements done.
            -- But we need to return only one value (the result).
            local res, _ = PROMPT_TEMPLATE:gsub(
                '@@@',
                string.format('What does "%s" mean?', luastatus.plugin.escape_double_quoted(fortune))
            )
            return res
        end,
    },
    cb = function(t)
        if t.what == 'answer' then
            if t.mu then
                return nil
            end
            local text = t.answer:gsub('\n', '')
            return luastatus.barlib.escape(text)
        elseif t.what == 'error' then
            return '%{F#e19f00}' .. '——×——' .. '%{F-}'
        elseif t.what == 'prompt_error' then
            error('got what="prompt_error"!')
        end
    end,
}
