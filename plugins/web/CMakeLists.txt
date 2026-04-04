file (GLOB sources "*.c" "mod_json/*.c" "mod_urlencode/*.c")
luastatus_add_plugin (
    plugin-web
    $<TARGET_OBJECTS:ls>
    $<TARGET_OBJECTS:moonvisit>
    $<TARGET_OBJECTS:safe>
    ${sources}
)

target_compile_definitions (plugin-web PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-web LUA)
target_include_directories (plugin-web PUBLIC "${PROJECT_SOURCE_DIR}")

find_library (MATH_LIBRARY m)
if (MATH_LIBRARY)
    target_link_libraries (plugin-web PUBLIC ${MATH_LIBRARY})
endif ()

find_package (PkgConfig REQUIRED)
pkg_check_modules (CURL_STUFF REQUIRED libcurl)
luastatus_target_build_with (plugin-web CURL_STUFF)
pkg_check_modules (CJSON_STUFF REQUIRED libcjson)
luastatus_target_build_with (plugin-web CJSON_STUFF)

luastatus_add_man_page (README.rst luastatus-plugin-web 7)
