widget = luastatus.require_plugin('file-contents-linux').widget{
    filename = os.getenv('HOME') .. '/status',
    cb = function(f)
        -- show the first line of the file
        return {full_text = f:read('*line')}
    end,
}
