file (GLOB sources "*.c")
add_executable (dbus_serv ${sources})
target_compile_definitions (dbus_serv PUBLIC -D_POSIX_C_SOURCE=200809L)

find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB_STUFF REQUIRED glib-2.0 gio-2.0)

target_include_directories (dbus_serv SYSTEM PUBLIC ${GLIB_STUFF_INCLUDE_DIRS})
target_compile_options (dbus_serv PUBLIC ${GLIB_STUFF_CFLAGS_OTHER})
target_link_libraries (dbus_serv PUBLIC ${GLIB_STUFF_LIBRARIES})
