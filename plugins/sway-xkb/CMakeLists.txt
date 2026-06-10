file (GLOB sources "*.c")
luastatus_add_plugin (
    plugin-sway-xkb
    $<TARGET_OBJECTS:ls>
    $<TARGET_OBJECTS:moonvisit>
    ${sources}
)

target_compile_definitions (plugin-sway-xkb PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-sway-xkb LUA)
target_include_directories (plugin-sway-xkb PUBLIC "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

###CHECK_INCLUDES_PRAGMA:MODULES=libcjson
###CHECK_INCLUDES_PRAGMA:EXTRA_CPPFLAGS=-DCJSON_FOUND_BY_PKG_CONFIG=1

pkg_check_modules (CJSON_STUFF libcjson)
if (CJSON_STUFF_FOUND)
    luastatus_target_build_with (plugin-sway-xkb CJSON_STUFF)
else ()
    message (WARNING "libcjson was not found by pkg-config; will try to simply link with -lcjson")
    target_link_libraries (plugin-sway-xkb PUBLIC -lcjson)
endif ()

set (CJSON_FOUND_BY_PKG_CONFIG "${CJSON_STUFF_FOUND}")
configure_file ("json_config.in.h" "json_config.generated.h")

luastatus_add_man_page (README.rst luastatus-plugin-sway-xkb 7)
