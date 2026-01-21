local months = {
    'января',
    'февраля',
    'марта',
    'апреля',
    'мая',
    'июня',
    'июля',
    'августа',
    'сентября',
    'октября',
    'ноября',
    'декабря',
}
widget = {
    plugin = 'timer',
    cb = function()
        local d = os.date('*t')
        return {
            string.format('%d %s', d.day, months[d.month]),
            string.format('%02d:%02d', d.hour, d.min),
        }
    end,
}
