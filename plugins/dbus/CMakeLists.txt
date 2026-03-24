file (GLOB sources "*.c" "zoo/*.c")
luastatus_add_plugin (plugin-dbus $<TARGET_OBJECTS:ls> $<TARGET_OBJECTS:moonvisit> ${sources})

target_compile_definitions (plugin-dbus PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-dbus LUA)
target_include_directories (plugin-dbus PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB_STUFF REQUIRED glib-2.0 gio-2.0)
luastatus_target_build_with (plugin-dbus GLIB_STUFF)

# find pthreads
set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
set (THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package (Threads REQUIRED)
# link against pthread
target_link_libraries (plugin-dbus PUBLIC Threads::Threads)

luastatus_add_man_page (README.rst                  luastatus-plugin-dbus 7)
luastatus_add_man_page (README_FN_CALL_METHOD.rst   luastatus-plugin-dbus-fn-call-method 7)
luastatus_add_man_page (README_FN_PROP.rst          luastatus-plugin-dbus-fn-prop 7)
luastatus_add_man_page (README_FN_MKVAL.rst         luastatus-plugin-dbus-fn-mkval 7)

# === lualib stuff below ===

function (add_stringified_file dest src target_name)
    add_custom_command (
        OUTPUT "${dest}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cstringify.sh" ARGS "${src}" "${dest}"
        MAIN_DEPENDENCY "${src}"
        VERBATIM)
    add_custom_target ("${target_name}" DEPENDS "${dest}")
endfunction ()

add_stringified_file (
    "${CMAKE_CURRENT_BINARY_DIR}/lualib.generated.inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/lualib.lua"
    "stringify-lualib"
)

add_dependencies (plugin-dbus "stringify-lualib")

target_include_directories (plugin-dbus PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
