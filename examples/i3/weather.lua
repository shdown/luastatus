local LOCATION_NAME = 'Moscow'

local TIMEOUT = 10

local INTERVAL = 9
local SLEEP_AFTER_INITIAL = 5

local coordinates = nil

local function planner()
    -- Get coordinates of LOCATION_NAME.
    while true do
        -- Form the URL
        local url = string.format(
            'https://geocoding-api.open-meteo.com/v1/search?name=%s&count=1',
            luastatus.plugin.urlencode(LOCATION_NAME)
        )
        -- Make request
        coroutine.yield({action = 'request', params = {
            url = url,
            timeout = TIMEOUT,
        }})
        -- If successful, break
        if coordinates ~= nil then
            break
        end
        -- Failure; sleep and retry
        coroutine.yield({action = 'sleep', period = INTERVAL})
    end

    -- Coordinates fetched, sleep for SLEEP_AFTER_INITIAL.
    coroutine.yield({action = 'sleep', period = SLEEP_AFTER_INITIAL})

    -- Form the weather URL
    local weather_url = string.format(
        'https://api.open-meteo.com/v1/forecast' ..
        '?latitude=%f' ..
        '&longitude=%f' ..
        '&current_weather=true',
        coordinates.latitude,
        coordinates.longitude
    )

    -- Fetch weather, periodically
    while true do
        -- Make request
        coroutine.yield({action = 'request', params = {
            url = weather_url,
            timeout = TIMEOUT,
        }})
        -- Sleep and repeat
        coroutine.yield({action = 'sleep', period = INTERVAL})
    end
end

local function check_error(t)
    if t.error then
        -- Low-level libcurl error
        return t.error
    end
    if t.status < 200 or t.status > 299 then
        -- Bad HTTP status
        return string.format('HTTP status %d', t.status)
    end
    -- Everything's OK
    return nil
end

local WEATHER_CODES = {
    [0] = 'Clear sky',

    [1] = 'Mainly clear',
    [2] = 'Partly cloudy',
    [3] = 'Overcast',

    [45] = 'Fog',
    [48] = 'Depositing rime fog',

    [51] = 'Drizzle, light',
    [53] = 'Drizzle, moderate',
    [55] = 'Drizzle, dense',

    [56] = 'Freezing drizzle, light',
    [57] = 'Freezing drizzle, dense',

    [61] = 'Rain, slight',
    [63] = 'Rain, moderate',
    [65] = 'Rain, heavy',

    [66] = 'Freezing rain, light',
    [67] = 'Freezing rain, heavy',

    [71] = 'Snow fall, slight',
    [73] = 'Snow fall, moderate',
    [75] = 'Snow fall, heavy',

    [77] = 'Snow grains',

    [80] = 'Rain shower, slight',
    [81] = 'Rain shower, moderate',
    [82] = 'Rain shower, violent',

    [85] = 'Snow showers, slight',
    [87] = 'Snow showers, heavy',

    [95] = 'Thunderstorm',
    [96] = 'Thunderstorm, slight hail',
    [99] = 'Thunderstorm, heavy hail',
}

local function fmt_weather(temp, _, _, weather_code)
    local segment1 = {full_text = string.format('%.0f°', temp)}
    local segment2 = nil
    local weather = WEATHER_CODES[weather_code]
    if weather then
        segment2 = {full_text = weather}
    end
    return {segment1, segment2}
end

widget = {
    plugin = 'web',
    opts = {
        planner = planner,
    },
    cb = function(t)
        local err_msg = check_error(t)
        if err_msg then
            print(string.format('WARNING: luastatus: weather widget: %s'), err_msg)
            return {full_text = '...', color = '#e03838'}
        end

        if coordinates == nil then
            local obj = assert(luastatus.plugin.json_decode(t.body))
            local result = assert(obj.results[1])
            local latitude = assert(result.latitude)
            local longitude = assert(result.longitude)
            coordinates = {latitude = latitude, longitude = longitude}
            return {full_text = '...', color = '#709080'}
        end

        local obj = assert(luastatus.plugin.json_decode(t.body))
        local cw = assert(obj.current_weather)

        local temp = assert(cw.temperature)
        local wind_speed = assert(cw.windspeed)
        -- is_day is an integer, either 0 or 1
        local is_day = assert(cw.is_day) ~= 0
        local weather_code = assert(cw.weathercode)

        return fmt_weather(temp, wind_speed, is_day, weather_code)
    end,
}
