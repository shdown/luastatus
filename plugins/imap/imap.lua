--[[
  Copyright (C) 2015-2020  luastatus developers

  This file is part of luastatus.

  luastatus is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  luastatus is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
--]]

local socket = require 'socket'
local ssl = require 'ssl'

local log = print

----------------------------------------------------------------------------------------------------

local IMAP_TIMEOUT_ERROR = {}

local IMAP = {}

function IMAP:open(host, port, params)
    local conn = socket.tcp()
    conn:connect(host, port)
    conn:settimeout(params.handshake_timeout)
    if params.use_ssl then
        conn = assert(ssl.wrap(conn, {
            mode = 'client',
            protocol = 'tlsv1',
            cafile = '/etc/ssl/certs/ca-certificates.crt',
            verify = 'peer',
            options = 'all',
        }))
        assert(conn:dohandshake())
    end
    conn:settimeout(params.timeout)
    self.__index = self
    return setmetatable({
        conn = conn,
        last_id = 1,
        verbose = params.verbose,
    }, self)
end

function IMAP:receive()
    local line, err = self.conn:receive()
    if line == nil then
        error(err == 'wantread' and IMAP_TIMEOUT_ERROR or err)
    end
    if self.verbose then
        log('<', line)
    end
    return line
end

function IMAP:send(str)
    if self.verbose then
        log('>', str)
    end
    local data = str .. '\r\n'
    local nsent = 0
    while nsent ~= #data do
        nsent = assert(self.conn:send(data, nsent + 1))
    end
end

function IMAP:command(query, ondata)
    local id = string.format('a%04d', self.last_id)
    self.last_id = self.last_id + 1
    local pattern = '^' .. id .. ' (%w+)'

    self:send(id .. ' ' .. query)

    while true do
        local line = self:receive()
        local resp = line:match(pattern)
        if resp ~= nil then
            return resp == 'OK', line
        end
        if ondata ~= nil then
            ondata(line)
        end
    end
end

function IMAP:close()
    self.conn:close()
end

----------------------------------------------------------------------------------------------------

local P = {}

function P.widget(tbl)
    local error_sleep_period = tbl.error_sleep_period
    -- to prevent busy-looping if an invalid value is passed to /luastatus.plugin.push_period/.
    assert(type(error_sleep_period) == 'number' and error_sleep_period > 0,
           'invalid "error_sleep_period" value')

    local mbox = nil

    local function connect()
        mbox = IMAP:open(tbl.host, tbl.port, {
            use_ssl = tbl.use_ssl,
            timeout = tbl.timeout,
            handshake_timeout = tbl.handshake_timeout,
            verbose = tbl.verbose,
        })

        assert(mbox:command(string.format('LOGIN %s %s', tbl.login, tbl.password)))
        assert(mbox:command('SELECT ' .. tbl.mailbox))
    end

    local function get_unseen()
        local unseen
        repeat
            local finish = true
            assert(mbox:command('STATUS ' .. tbl.mailbox .. ' (UNSEEN)', function(line)
                if not line:match('^%*') then
                    return
                end
                local m = line:match('^%* STATUS .* %(UNSEEN (%d+)%)$')
                if m then
                    unseen = m
                else
                    finish = false
                end
            end))
        until finish
        assert(unseen ~= nil)
        return tonumber(unseen)
    end

    local function idle()
        local done_sent = false
        assert(mbox:command('IDLE', function(line)
            if not done_sent and line:match('^%*') then
                mbox:send('DONE')
                done_sent = true
            end
        end))
    end

    local function iteration()
        if mbox == nil then
            connect()
        else
            idle()
        end
        return get_unseen()
    end

    local last_content = tbl.cb(nil)
    return {
        plugin = 'timer',
        opts = {period = 0},
        cb = function()
            local is_ok, obj = pcall(iteration)
            if is_ok then
                last_content = tbl.cb(obj)
                return last_content
            end

            if mbox ~= nil then
                mbox:close()
                mbox = nil
            end

            if obj == IMAP_TIMEOUT_ERROR then
                return last_content
            else
                if tbl.verbose then
                    log('!', obj)
                end
                luastatus.plugin.push_period(error_sleep_period)
                last_content = tbl.cb(nil)
                return last_content
            end
        end,
        event = tbl.event,
    }
end

return P
