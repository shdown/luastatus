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
    [0] = 'Ясно',

    [1] = 'Преимущественно ясно',
    [2] = 'Переменная облачность',
    [3] = 'Пасмурно',

    [45] = 'Туман',
    [48] = 'Осаждение инея и тумана',

    [51] = 'Изморось, лёгкая',
    [53] = 'Изморось, умеренная',
    [55] = 'Изморось, сильная',

    [56] = 'Ледяная изморось, лёгкая',
    [57] = 'Ледяная изморось, сильная',

    [61] = 'Дождь, лёгкий',
    [63] = 'Дождь, умеренный',
    [65] = 'Дождь, сильный',

    [66] = 'Град, лёгкий',
    [67] = 'Град, сильный',

    [71] = 'Снегопад, лёгкий',
    [73] = 'Снегопад, умеренный',
    [75] = 'Снегопад, сильный',

    [77] = 'Крупинки снега',

    [80] = 'Ливень, лёгкий',
    [81] = 'Ливень, умеренный',
    [82] = 'Ливень, сильный',

    [85] = 'Снеговые осадки, лёгкие',
    [87] = 'Снеговые осадки, сильные',

    [95] = 'Гроза',
    [96] = 'Гроза, небольшой град',
    [99] = 'Гроза, сильный град',
}

local function fmt_weather(temp, _, _, weather_code)
    local segment1 = string.format('%.0f°', temp)
    -- segment2 is either string or nil
    local segment2 = WEATHER_CODES[weather_code]
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
            return '<!>'
        end

        if coordinates == nil then
            local obj = assert(luastatus.plugin.json_decode(t.body))
            local result = assert(obj.results[1])
            local latitude = assert(result.latitude)
            local longitude = assert(result.longitude)
            coordinates = {latitude = latitude, longitude = longitude}
            return '...'
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
