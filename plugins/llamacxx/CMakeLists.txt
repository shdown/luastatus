file (GLOB sources "*.c")
luastatus_add_plugin (
    plugin-llamacxx
    $<TARGET_OBJECTS:ls>
    $<TARGET_OBJECTS:moonvisit>
    $<TARGET_OBJECTS:runshell>
    $<TARGET_OBJECTS:widechar>
    ${sources}
)

configure_file ("config.in.h" "config.generated.h")

target_compile_definitions (plugin-llamacxx PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-llamacxx LUA)
target_include_directories (plugin-llamacxx PUBLIC "${PROJECT_SOURCE_DIR}")

find_library (MATH_LIBRARY m)
if (MATH_LIBRARY)
    target_link_libraries (plugin-llamacxx PUBLIC ${MATH_LIBRARY})
endif ()

find_package (PkgConfig REQUIRED)
pkg_check_modules (CURL_STUFF REQUIRED libcurl)
luastatus_target_build_with (plugin-llamacxx CURL_STUFF)
pkg_check_modules (YAJL_STUFF REQUIRED yajl)
luastatus_target_build_with (plugin-llamacxx YAJL_STUFF)

luastatus_add_man_page (README.rst luastatus-plugin-llamacxx 7)
