file (GLOB sources "*.c")
luastatus_add_plugin (plugin-alsa $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (plugin-alsa PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-alsa LUA)
target_include_directories (plugin-alsa PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (ALSA REQUIRED alsa)
luastatus_target_build_with (plugin-alsa ALSA)

luastatus_add_man_page (README.rst luastatus-plugin-alsa 7)
