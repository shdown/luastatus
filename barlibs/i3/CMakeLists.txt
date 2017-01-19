file (GLOB sources "*.c")
luastatus_add_barlib (i3 $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (i3 PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (i3 LUA)
target_include_directories (i3 PUBLIC "${PROJECT_SOURCE_DIR}")

find_package (PkgConfig REQUIRED)
pkg_check_modules (YAJL REQUIRED yajl>=2.1.0)
luastatus_target_build_with (i3 YAJL)

include(GNUInstallDirs)

install (FILES luastatus-i3-wrapper DESTINATION ${CMAKE_INSTALL_BINDIR})
