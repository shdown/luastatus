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

local last_fortune = ''

widget = {
    plugin = 'llamacxx',
    opts = {
        prompt = function()
            local fortune = FORTUNES[math.random(1, #FORTUNES)]
            last_fortune = fortune
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
            local body = string.format('[%s] => [%s]', last_fortune, t.answer:gsub('\n', ' '))
            last_fortune = ''
            return {full_text = body}
        elseif t.what == 'error' then
            return {full_text = '——×——', color = '#e19f00'}
        elseif t.what == 'prompt_error' then
            error('got what="prompt_error"!')
        end
    end,
}
