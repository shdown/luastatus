-- So, it looks like this:
--
--     ••• | ██████████████       | [ 70%] | •••
--
--           ^ gauge block          ^ text block
--
-- The gauge block is initially hidden; click on the text block to toggle its visibility.
-- Click anywhere at the gauge block to change the volume.
-- Click with the right mouse button on either block to toggle mute state.

local CARD = 'default'
local CHANNEL = 'Master'

local HBLOCKS = {' ', '▏', '▎', '▍', '▌', '▋', '▊', '▉', '█'}
local GAUGE_NCHARS = 20

-- i3bar adds the separator width to "relative_x" and "width" properties of a click event.
--
-- So let's do some multivariable equations (we assume a monospace font is used).
--
-- Let:
--     * `x' be the separator width in pixels (unknown);
--     * `y' be the width of one character in pixels (unknown);
--     * `U' be the number of characters in the text block (known);
--     * `V' be the number of characters in the gauge block (known);
--     * `A' be the width, in pixels, of the text block (known when clicked);
--     * `B' be the width, in pixels, on the gauge block (known when clicked).
--
-- We have:
--     { x + U*y = A;
--     { x + V*y = B.
-- So,
--     y = (A - B) / (U - V);
--     x = A - U*y.

local last_t = nil
local text_block_nchars, text_block_width = nil, nil
local gauge = false

local function round(x)
    return math.floor(x + 0.5)
end

local function mk_gauge(level)
    local rel_level = level * GAUGE_NCHARS
    local nfull = math.floor(rel_level)
    local filled = HBLOCKS[#HBLOCKS]:rep(nfull)
    if nfull == GAUGE_NCHARS then
        return filled
    end
    local mid_idx = round((rel_level - nfull) * (#HBLOCKS - 1))
    return filled .. HBLOCKS[1 + mid_idx] .. HBLOCKS[1]:rep(GAUGE_NCHARS - nfull - 1)
end

widget = {
    plugin = 'alsa',

    opts = {
        card = CARD,
        channel = CHANNEL,
        make_self_pipe = true,
    },

    cb = function(t)
        last_t = t

        local level = (t.vol.cur - t.vol.min) / (t.vol.max - t.vol.min)

        local r = {}
        if t.mute then
            r[2] = {full_text='[mute]', color='#e03838'}
        else
            r[2] = {full_text=string.format('[%3d%%]', round(level * 100)), color='#718ba6'}
        end
        text_block_nchars = #r[2].full_text -- please note this does not work with Unicode.

        if gauge then
            local fg, bg = '#dcdcdc', '#444444'
            if t.mute then
                fg, bg = '#e03838', '#4a1414'
            end
            r[1] = {full_text=mk_gauge(level), color=fg, background=bg, instance='gauge'}
        end

        return r
    end,

    event = function(t)
        if t.button == 1 then -- left mouse button
            if t.instance == 'gauge' then
                local char_width = round((t.width - text_block_width)
                                         / (GAUGE_NCHARS - text_block_nchars))
                local sep_width = text_block_width - text_block_nchars * char_width

                local x = t.relative_x - sep_width
                if x < 0 then
                    return
                end
                local rawvol = round(
                    last_t.vol.min +
                    x / (t.width - sep_width) * (last_t.vol.max - last_t.vol.min))
                assert(os.execute(
                    string.format('amixer -D "%s" set "%s" %s >/dev/null', CARD, CHANNEL, rawvol)))

            else
                gauge = not gauge
                text_block_width = t.width
                luastatus.plugin.wake_up()
            end

        elseif t.button == 3 then -- right mouse button
            assert(os.execute(string.format(
                'amixer -D "%s" set "%s" toggle >/dev/null', CARD, CHANNEL)))
        end
    end,
}
