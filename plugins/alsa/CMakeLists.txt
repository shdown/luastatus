file (GLOB sources "*.c")
luastatus_add_plugin (alsa $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (alsa PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (alsa LUA)
target_include_directories (alsa PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (ALSA REQUIRED alsa)
luastatus_target_build_with (alsa ALSA)
