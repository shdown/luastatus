-- A Rube Goldberg machine widget that updates itself on click.

fifo_path = assert(os.getenv('HOME')) .. '/.luastatus-timer-fifo'

assert(0 == luastatus.rc{
    '/bin/sh',
    '-c',
    'set -e; rm -f -- "$1"; mkfifo -m600 -- "$1"',
    '',        -- this is $0
    fifo_path, -- and this is $1
})

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
        luastatus.spawn{'touch', '--', fifo_path}
    end,
}
