file (GLOB sources "*.c")
luastatus_add_plugin (plugin-xtitle $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (plugin-xtitle PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-xtitle LUA)
target_include_directories (plugin-xtitle PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (XCBLIBS REQUIRED xcb xcb-ewmh xcb-icccm xcb-event)
luastatus_target_build_with (plugin-xtitle XCBLIBS)

luastatus_add_man_page (README.rst luastatus-plugin-xtitle 7)
