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
FORTUNES = load_fortunes()

PROMPT_TEMPLATE = [[<start_of_turn>user
You are a helpful assistant

Hello<end_of_turn>
<start_of_turn>model
Hi there<end_of_turn>
<start_of_turn>user
@@@<end_of_turn>
<start_of_turn>model
]]

T_MIN, T_MAX = 0.8, 2.0

local function get_divisor(tn)
    if tn == 1 then
        return 1
    end
    return tn / 1.25
end

local function mk_extra_params_json(tn)
    assert(tn >= 1)
    local t = T_MAX - (T_MAX - T_MIN) / get_divisor(tn)
    return string.format('"temperature":%.10f', t)
end

local function mk_color(tn)
    assert(tn >= 1)
    local red = math.floor(255 - 255 / get_divisor(tn))
    return string.format('#%02x%02x00', red, 255 - red)
end

local tn_cur = 1
local is_timeout_because_clicked = false

local last_fortune = ''

SEGMENT_DOTS = {full_text = '•••••', color = '#e19f00'}

widget = {
    plugin = 'llamacxx',
    opts = {
        prompt = function()
            local fortune = FORTUNES[math.random(1, #FORTUNES)]
            last_fortune = fortune
            -- string.gsub returns two values: result and number of replacements done.
            -- But we need to return only one value (the result).
            local res, n = PROMPT_TEMPLATE:gsub(
                '@@@',
                string.format('What does "%s" mean?', luastatus.plugin.escape_double_quoted(fortune))
            )
            return res
        end,
        extra_params_json = mk_extra_params_json(1),
        tell_about_timeout = true,
        make_self_pipe = true,
        upd_timeout = 10,
    },
    cb = function(t)

        if t.what == 'answer' then
            local body = string.format('[%s] => [%s]', last_fortune, t.answer:gsub('\n', ' '))
            last_fortune = ''
            return {full_text = body, color = mk_color(tn_cur)}

        elseif t.what == 'timeout' then
            if is_timeout_because_clicked then
                is_timeout_because_clicked = false
            else
                tn_cur = 1
            end
            return SEGMENT_DOTS

        elseif t.what == 'self_pipe' then
            tn_cur = tn_cur + 1

            luastatus.plugin.push_timeout(0)
            luastatus.plugin.push_extra_params_json(mk_extra_params_json(tn_cur))
            is_timeout_because_clicked = true
            return SEGMENT_DOTS

        elseif t.what == 'error' then
            return {full_text = '——×——', color = '#e19f00'}

        elseif t.what == 'prompt_error' then
            error('got what="prompt_error"!')
        end
    end,
    event = function(t)
        if t.button == 1 then
            luastatus.plugin.wake_up()
        end
    end,
}
