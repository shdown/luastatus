file (GLOB sources "*.c")
luastatus_add_plugin (xkb $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (xkb PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (xkb LUA)
target_include_directories (xkb PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (X11 REQUIRED x11)
luastatus_target_build_with (xkb X11)
