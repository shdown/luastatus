file (GLOB sources "*.c")
luastatus_add_plugin (dbus $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (dbus PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (dbus LUA)
target_include_directories (dbus PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB_STUFF REQUIRED glib-2.0 gio-2.0)
luastatus_target_build_with (dbus GLIB_STUFF)
