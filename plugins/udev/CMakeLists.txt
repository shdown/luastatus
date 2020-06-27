file (GLOB sources "*.c")
luastatus_add_plugin (plugin-udev $<TARGET_OBJECTS:ls> $<TARGET_OBJECTS:moonvisit> ${sources})

target_compile_definitions (plugin-udev PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-udev LUA)
target_include_directories (plugin-udev PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (UDEV REQUIRED libudev)
luastatus_target_build_with (plugin-udev UDEV)

luastatus_add_man_page (README.rst luastatus-plugin-udev 7)
