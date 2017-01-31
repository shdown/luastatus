lib = require "lib"

widget = {
  plugin = 'timer',
  opts = { period = 5 },
  cb = function(t)
    local days = {
      'Понедельник', 
      'Вторник', 
      'Среда', 
      'Четверг', 
      'Пятница', 
      'Суббота', 
      'Воскресенье',
    }
    local months = { 
      'января',  
      'февраля', 
      'марта', 
      'апреля', 
      'мая', 
      'июня', 
      'июля', 
      'августа', 
      'сентрября', 
      'октября', 
      'ноября', 
      'декабря',
    }
    local d = os.date('*t')
    local text = string.format(' %s, %d %s, %02d:%02d ', days[d.wday], d.day, months[d.month], d.hour, d.min)
    return lib.colorize(text, '#eee', '#000')
  end
}
