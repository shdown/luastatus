file (GLOB sources "*.c")
luastatus_add_plugin (
    plugin-xkb
    $<TARGET_OBJECTS:ls>
    $<TARGET_OBJECTS:moonvisit>
    $<TARGET_OBJECTS:safe>
    ${sources}
)

target_compile_definitions (plugin-xkb PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-xkb LUA)
target_include_directories (plugin-xkb PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (xstuff REQUIRED x11)
luastatus_target_build_with (plugin-xkb xstuff)

luastatus_add_man_page (README.rst luastatus-plugin-xkb 7)
