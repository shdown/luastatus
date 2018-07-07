cmake_minimum_required (VERSION 3.0.0)
project (luastatus C)

#------------------------------------------------------------------------------

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif ()

#------------------------------------------------------------------------------

set (CMAKE_C_STANDARD 99)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"
)
    if (${CMAKE_BUILD_TYPE} MATCHES "Release|MinSizeRel")
        set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
    endif ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall -Wextra -Wcast-qual -Wcast-align -Wfloat-equal -Wredundant-decls -Wshadow -Wpointer-arith -Wstrict-prototypes -Winit-self")
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wlogical-op")
    endif ()
endif ()

#------------------------------------------------------------------------------

option (NO_PKGCONFIG "Fall back to CMake's find_package(...) instead of using pkg-config" OFF)

if (NO_PKGCONFIG)
    if (WITH_LUA_VERSION)
        find_package (Lua "${WITH_LUA_VERSION}" EXACT REQUIRED)
    else ()
        find_package (Lua 5.1 REQUIRED) # 5.1 is the minimum version required
    endif ()
    set (LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
    set (LUA_CFLAGS_OTHER "")
else ()
    find_package (PkgConfig REQUIRED)
    if (WITH_LUA_LIBRARY)
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
endif ()

function (luastatus_target_compile_with target var)
    target_include_directories (${target} SYSTEM PUBLIC
        ${${var}_INCLUDE_DIRS})
    target_compile_options (${target} PUBLIC
        ${${var}_CFLAGS_OTHER})
endfunction ()

function (luastatus_target_build_with target var)
    luastatus_target_compile_with ("${target}" "${var}")
    target_link_libraries (${target}
        ${${var}_LIBRARIES})
endfunction ()

#------------------------------------------------------------------------------
include (GNUInstallDirs)

set (BARLIBS_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/luastatus/barlibs")
set (PLUGINS_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/luastatus/plugins")

function (luastatus_add_barlib_or_plugin destdir name)
    set (sources ${ARGV})
    list (REMOVE_AT sources 0 1)
    add_library ("${name}" MODULE ${sources})
    set_target_properties ("${name}" PROPERTIES PREFIX "")
    if (destdir)
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
    luastatus_add_barlib_or_plugin ("" ${ARGV})
endfunction ()

function (luastatus_add_plugin_noinstall)
    luastatus_add_barlib_or_plugin ("" ${ARGV})
endfunction ()

#------------------------------------------------------------------------------

option (BUILD_BARLIB_DWM "build barlibs/dwm" ON)
option (BUILD_BARLIB_I3 "build barlibs/i3" ON)
option (BUILD_BARLIB_LEMONBAR "build barlibs/lemonbar" ON)

option (BUILD_PLUGIN_ALSA "build plugins/alsa" ON)
option (BUILD_PLUGIN_BATTERY_LINUX "build plugins/battery-linux" ON)
option (BUILD_PLUGIN_CPU_USAGE_LINUX "build plugins/cpu-usage-linux" ON)
option (BUILD_PLUGIN_DBUS "build plugins/dbus" ON)
option (BUILD_PLUGIN_FILE_CONTENTS_LINUX "build plugins/file-contents-linux" ON)
option (BUILD_PLUGIN_FS "build plugins/fs" ON)
option (BUILD_PLUGIN_INOTIFY "build plugins/inotify" ON)
option (BUILD_PLUGIN_IMAP "build plugins/imap" ON)
option (BUILD_PLUGIN_MEM_USAGE_LINUX "build plugins/mem-usage-linux" ON)
option (BUILD_PLUGIN_MPD "build plugins/mpd" ON)
option (BUILD_PLUGIN_PIPE "build plugins/pipe" ON)
option (BUILD_PLUGIN_TIMER "build plugins/timer" ON)
option (BUILD_PLUGIN_XKB "build plugins/xkb" ON)
option (BUILD_PLUGIN_XTITLE "build plugins/xtitle" ON)

#------------------------------------------------------------------------------

add_subdirectory (libls)
add_subdirectory (luastatus)
add_subdirectory (tests)

if (BUILD_BARLIB_DWM)
    add_subdirectory (barlibs/dwm)
endif ()

if (BUILD_BARLIB_I3)
    add_subdirectory (barlibs/i3)
endif ()

if (BUILD_BARLIB_LEMONBAR)
    add_subdirectory (barlibs/lemonbar)
endif ()

if (BUILD_PLUGIN_ALSA)
    add_subdirectory (plugins/alsa)
endif ()

if (BUILD_PLUGIN_BATTERY_LINUX)
    add_subdirectory (plugins/battery-linux)
endif ()

if (BUILD_PLUGIN_CPU_USAGE_LINUX)
    add_subdirectory (plugins/cpu-usage-linux)
endif ()

if (BUILD_PLUGIN_DBUS)
    add_subdirectory (plugins/dbus)
endif ()

if (BUILD_PLUGIN_FILE_CONTENTS_LINUX)
    add_subdirectory (plugins/file-contents-linux)
endif ()

if (BUILD_PLUGIN_FS)
    add_subdirectory (plugins/fs)
endif ()

if (BUILD_PLUGIN_IMAP)
    add_subdirectory (plugins/imap)
endif ()

if (BUILD_PLUGIN_INOTIFY)
    add_subdirectory (plugins/inotify)
endif ()

if (BUILD_PLUGIN_MEM_USAGE_LINUX)
    add_subdirectory (plugins/mem-usage-linux)
endif ()

if (BUILD_PLUGIN_MPD)
    add_subdirectory (plugins/mpd)
endif ()

if (BUILD_PLUGIN_PIPE)
    add_subdirectory (plugins/pipe)
endif ()

if (BUILD_PLUGIN_TIMER)
    add_subdirectory (plugins/timer)
endif ()

if (BUILD_PLUGIN_XKB)
    add_subdirectory (plugins/xkb)
endif ()

if (BUILD_PLUGIN_XTITLE)
    add_subdirectory (plugins/xtitle)
endif ()
