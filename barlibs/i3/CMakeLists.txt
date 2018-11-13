file (GLOB sources "*.c")
luastatus_add_barlib (barlib-i3 $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (barlib-i3 PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (barlib-i3 LUA)
target_include_directories (barlib-i3 PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (YAJL REQUIRED yajl>=2.0.4)
luastatus_target_build_with (barlib-i3 YAJL)

include (GNUInstallDirs)

install (PROGRAMS luastatus-i3-wrapper DESTINATION ${CMAKE_INSTALL_BINDIR})

luastatus_add_man_page (README.rst luastatus-barlib-i3 7)
