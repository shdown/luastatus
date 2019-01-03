file (GLOB sources "*.c")
luastatus_add_plugin (plugin-wireless $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (plugin-wireless PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-wireless LUA)
target_include_directories (plugin-wireless PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (NL_AND_CO REQUIRED libnl-3.0 libnl-genl-3.0)
luastatus_target_build_with (plugin-wireless NL_AND_CO)

luastatus_add_man_page (README.rst luastatus-plugin-wireless 7)
