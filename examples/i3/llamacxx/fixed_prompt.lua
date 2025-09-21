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
            return {full_text = t.answer:gsub('\n', ' ')}
        elseif t.what == 'error' then
            return {full_text = '——×——', color = '#e19f00'}
        elseif t.what == 'prompt_error' then
            error('got what="prompt_error"!')
        end
    end,
}
