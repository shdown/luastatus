dofile 'lib.lua'

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

widget = {
  plugin = 'timer',
  opts = { period = 5 },
  cb = function(t)
    local text = ' ' .. days[tonumber(os.date('%u'))] ..
      os.date(', %d ') .. months[tonumber(os.date('%m'))] ..
      os.date(', %H:%M ')
    return colorize(text, '#eee', '#000')
  end
}
