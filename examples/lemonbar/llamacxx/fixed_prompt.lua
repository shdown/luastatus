PROMPT_TEMPLATE = [[<start_of_turn>user
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
            -- string.gsub returns two values: result and number of replacements done.
            -- But we need to return only one value (the result).
            local res, n = PROMPT_TEMPLATE:gsub('@@@', 'What is the purpose of life?')
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
