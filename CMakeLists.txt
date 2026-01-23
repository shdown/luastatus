cmake_minimum_required (VERSION 3.1.3...3.10)
project (luastatus C)

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif ()

set (CMAKE_C_STANDARD 99)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

#------------------------------------------------------------------------------

option (WITH_UBSAN "build with UBSAN (undefined behavior sanitizer)" OFF)
if (${WITH_UBSAN})
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -fsanitize-trap=all -fno-sanitize-recover=all")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=local-bounds")
    endif ()
endif ()

option (WITH_ASAN "build with ASAN (address sanitizer)" OFF)
if (${WITH_ASAN})
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-trap=all -fno-sanitize-recover=all")
endif ()

option (WITH_TSAN "build with TSAN (thread sanitizer)" OFF)
if (${WITH_TSAN})
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -fsanitize-trap=all -fno-sanitize-recover=all")
endif ()

option (WITH_LSAN "build with LSAN (memory leak sanitizer)" OFF)
if (${WITH_LSAN})
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak -fsanitize-trap=all -fno-sanitize-recover=all")
endif ()

#------------------------------------------------------------------------------

find_package (PkgConfig REQUIRED)
set (WITH_LUA_LIBRARY "-" CACHE STRING "Lua library name")
if (NOT "${WITH_LUA_LIBRARY}" STREQUAL "-")
    pkg_search_module (LUA REQUIRED "${WITH_LUA_LIBRARY}")
else ()
    pkg_search_module (LUA REQUIRED
        lua54 lua-5.4 lua5.4
        lua53 lua-5.3 lua5.3
        lua52 lua-5.2 lua5.2
        lua51 lua-5.1 lua5.1
        luajit
        lua)
endif ()

function (luastatus_target_compile_with target var)
    target_include_directories (${target} SYSTEM PUBLIC
        ${${var}_INCLUDE_DIRS})
    target_compile_options (${target} PUBLIC
        ${${var}_CFLAGS_OTHER})
endfunction ()

function (luastatus_target_build_with target var)
    luastatus_target_compile_with ("${target}" "${var}")
    target_link_libraries (${target} PUBLIC ${${var}_LIBRARIES})
endfunction ()

#------------------------------------------------------------------------------

include (GNUInstallDirs)

set (BARLIBS_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/luastatus/barlibs")
set (PLUGINS_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/luastatus/plugins")
set (LUA_PLUGINS_DIR "${CMAKE_INSTALL_FULL_DATAROOTDIR}/luastatus/plugins")

function (luastatus_add_barlib_or_plugin destdir name)
    set (sources ${ARGV})
    list (REMOVE_AT sources 0 1)
    add_library ("${name}" MODULE ${sources})
    set_target_properties ("${name}" PROPERTIES PREFIX "")
    if (NOT "${destdir}" STREQUAL "-")
        install (TARGETS "${name}" DESTINATION "${destdir}")
    endif ()
endfunction ()

function (luastatus_add_barlib)
    luastatus_add_barlib_or_plugin ("${BARLIBS_DIR}" ${ARGV})
endfunction ()

function (luastatus_add_plugin)
    luastatus_add_barlib_or_plugin ("${PLUGINS_DIR}" ${ARGV})
endfunction ()

function (luastatus_add_barlib_noinstall)
    luastatus_add_barlib_or_plugin ("-" ${ARGV})
endfunction ()

function (luastatus_add_plugin_noinstall)
    luastatus_add_barlib_or_plugin ("-" ${ARGV})
endfunction ()

option (BUILD_DOCS "build man pages" ON)

function (luastatus_add_man_page src basename section)
    if (NOT BUILD_DOCS)
        return ()
    endif ()
    set (dest "${basename}.${section}")
    add_custom_command (
        OUTPUT "${dest}"
        COMMAND "${PROJECT_SOURCE_DIR}/generate-man.sh" ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${src}" "${dest}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        MAIN_DEPENDENCY "${src}"
        VERBATIM)
    add_custom_target ("man${section}-${basename}" ALL DEPENDS "${dest}")
    install (
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${dest}"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man${section}")
endfunction ()

#------------------------------------------------------------------------------

add_subdirectory (libls)
add_subdirectory (libsafe)
add_subdirectory (libhackyfix)
add_subdirectory (librunshell)
add_subdirectory (libmoonvisit)
add_subdirectory (libwidechar)
add_subdirectory (libprocalive)
add_subdirectory (luastatus)

#------------------------------------------------------------------------------

macro (DEF_OPT optname subdir defvalue)
    option (${optname} "build ${subdir}" ${defvalue})
    if (${optname})
        add_subdirectory (${subdir})
    endif ()
endmacro ()

DEF_OPT (BUILD_BARLIB_DWM                 "barlibs/dwm"                 ON)
DEF_OPT (BUILD_BARLIB_I3                  "barlibs/i3"                  ON)
DEF_OPT (BUILD_BARLIB_LEMONBAR            "barlibs/lemonbar"            ON)
DEF_OPT (BUILD_BARLIB_STDOUT              "barlibs/stdout"              ON)

DEF_OPT (BUILD_PLUGIN_ALSA                "plugins/alsa"                ON)
DEF_OPT (BUILD_PLUGIN_BACKLIGHT_LINUX     "plugins/backlight-linux"     ON)
DEF_OPT (BUILD_PLUGIN_BATTERY_LINUX       "plugins/battery-linux"       ON)
DEF_OPT (BUILD_PLUGIN_CPU_USAGE_LINUX     "plugins/cpu-usage-linux"     ON)
DEF_OPT (BUILD_PLUGIN_CPU_FREQ_LINUX      "plugins/cpu-freq-linux"      ON)
DEF_OPT (BUILD_PLUGIN_TEMPERATURE_LINUX   "plugins/temperature-linux"   ON)
DEF_OPT (BUILD_PLUGIN_DBUS                "plugins/dbus"                ON)
DEF_OPT (BUILD_PLUGIN_DISK_IO_LINUX       "plugins/disk-io-linux"       ON)
DEF_OPT (BUILD_PLUGIN_FILE_CONTENTS_LINUX "plugins/file-contents-linux" ON)
DEF_OPT (BUILD_PLUGIN_FS                  "plugins/fs"                  ON)
DEF_OPT (BUILD_PLUGIN_IMAP                "plugins/imap"                ON)
DEF_OPT (BUILD_PLUGIN_INOTIFY             "plugins/inotify"             ON)
DEF_OPT (BUILD_PLUGIN_IS_PROGRAM_RUNNING  "plugins/is-program-running"  ON)
DEF_OPT (BUILD_PLUGIN_MEM_USAGE_LINUX     "plugins/mem-usage-linux"     ON)
DEF_OPT (BUILD_PLUGIN_MPD                 "plugins/mpd"                 ON)
DEF_OPT (BUILD_PLUGIN_NETWORK_LINUX       "plugins/network-linux"       ON)
DEF_OPT (BUILD_PLUGIN_NETWORK_RATE_LINUX  "plugins/network-rate-linux"  ON)
DEF_OPT (BUILD_PLUGIN_PIPE                "plugins/pipe"                ON)
DEF_OPT (BUILD_PLUGIN_PULSE               "plugins/pulse"               OFF)
DEF_OPT (BUILD_PLUGIN_TIMER               "plugins/timer"               ON)
DEF_OPT (BUILD_PLUGIN_UDEV                "plugins/udev"                ON)
DEF_OPT (BUILD_PLUGIN_UNIXSOCK            "plugins/unixsock"            OFF)
DEF_OPT (BUILD_PLUGIN_XKB                 "plugins/xkb"                 ON)
DEF_OPT (BUILD_PLUGIN_XTITLE              "plugins/xtitle"              ON)
DEF_OPT (BUILD_PLUGIN_LLAMACXX            "plugins/llamacxx"            OFF)

DEF_OPT (BUILD_TESTS                      "tests"                       OFF)
