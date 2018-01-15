-- A Rube Goldberg machine widget that updates itself on click.

fifo_path = '~/.luastatus-timer-fifo'
assert(os.execute('set -e; F=' .. fifo_path .. '; rm -f -- $F; mkfifo -m600 -- $F'))

widget = {
    plugin = 'timer',
    opts = {
        fifo = fifo_path,
        period = 2.0,
    },
    cb = function(t)
        if t == 'fifo' then
            return {full_text = 'Thanks!'}
        else
            return {full_text = 'Click me'}
        end
    end,
    event = function(t)
        -- opening the FIFO for writing here may, in some rare cases, block (and doing this from
        -- this process would lead to a deadlock), so we spawn another process to do it and do not
        -- wait for its termination (which would also lead to a deadlock).
        os.execute('exec touch -- ' .. fifo_path .. '&')
    end,
}
