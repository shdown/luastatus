file (GLOB sources "*.c")
luastatus_add_barlib (barlib-dwm $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (barlib-dwm PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (barlib-dwm LUA)
target_include_directories (barlib-dwm PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (XCB REQUIRED xcb)
luastatus_target_build_with (barlib-dwm XCB)
