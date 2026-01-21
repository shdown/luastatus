local HIDE_IF_HAVE_MORE_UPDATES = false

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
        data_sources = {
            xtitle = [[
                LL_data_source = {
                    plugin = 'xtitle',
                    opts = {
                        extended_fmt = true,
                    },
                    cb = function(t)
                        return t.instance or ''
                    end,
                }
            ]],
        },
        prompt = function(data)
            if data.xtitle == '' then
                return nil
            end
            -- string.gsub returns two values: result and number of replacements done
            local res, _ = PROMPT_TEMPLATE:gsub(
                '@@@',
                string.format('How do I use program "%s"?', luastatus.plugin.escape_double_quoted(data.xtitle))
            )
            return res
        end,
        extra_params_json = '"max_tokens":100',
        report_mu = HIDE_IF_HAVE_MORE_UPDATES,
    },
    cb = function(t)
        if t.what == 'answer' then
            if t.mu then
                return nil
            end
            return {full_text = t.answer:gsub('\n', '')}
        elseif t.what == 'error' then
            return {full_text = '——×——', color = '#e19f00'}
        elseif t.what == 'prompt_error' then
            error('got what="prompt_error"!')
        end
    end,
}
