file (GLOB sources "*.c")
luastatus_add_plugin (xtitle $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (xtitle PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (xtitle LUA)
target_include_directories (xtitle PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (XCBLIBS REQUIRED xcb xcb-ewmh xcb-icccm)
luastatus_target_build_with (xtitle XCBLIBS)
