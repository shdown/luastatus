months = {'января', 'февраля', 'марта', 'апреля', 'мая', 'июня', 'июля', 'августа', 'сентября',
    'октября', 'ноября', 'декабря'}
widget = {
    plugin = 'timer',
    cb = function()
        local d = os.date('*t')
        return {
            {full_text = string.format('%d %s', d.day, months[d.month])},
            {full_text = string.format('%d:%02d', d.hour, d.min)},
        }
    end,
}
