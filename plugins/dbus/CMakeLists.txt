file (GLOB sources "*.c")
luastatus_add_plugin (plugin-dbus $<TARGET_OBJECTS:ls> $<TARGET_OBJECTS:moonvisit> ${sources})

target_compile_definitions (plugin-dbus PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-dbus LUA)
target_include_directories (plugin-dbus PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB_STUFF REQUIRED glib-2.0 gio-2.0)
luastatus_target_build_with (plugin-dbus GLIB_STUFF)

luastatus_add_man_page (README.rst luastatus-plugin-dbus 7)
