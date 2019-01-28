file (GLOB sources "*.c")
luastatus_add_plugin (plugin-pulse $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (plugin-pulse PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-pulse LUA)
target_include_directories (plugin-pulse PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (PULSE REQUIRED libpulse)
luastatus_target_build_with (plugin-pulse PULSE)

luastatus_add_man_page (README.rst luastatus-plugin-pulse 7)
