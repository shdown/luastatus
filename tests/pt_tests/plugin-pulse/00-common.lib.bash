main_fifo_file=./tmp-fifo-main
sink_name='PX7NE64ItQVFBw2'

x_pulse_begin() {
    pt_dbus_daemon_spawn --session
}

x_pulse_end() {
    pt_dbus_daemon_kill
}
