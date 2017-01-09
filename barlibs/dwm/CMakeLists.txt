file (GLOB sources "*.c")
luastatus_add_barlib (dwm $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (dwm PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (dwm LUA)
target_include_directories (dwm PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (XCB REQUIRED xcb)
luastatus_target_build_with (dwm XCB)
