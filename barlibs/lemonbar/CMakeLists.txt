file (GLOB sources "*.c")
luastatus_add_barlib (
    barlib-lemonbar
    $<TARGET_OBJECTS:ls>
    $<TARGET_OBJECTS:safe>
    ${sources}
)

target_compile_definitions (barlib-lemonbar PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (barlib-lemonbar LUA)
target_include_directories (barlib-lemonbar PUBLIC "${PROJECT_SOURCE_DIR}")

include (GNUInstallDirs)

install (PROGRAMS luastatus-lemonbar-launcher DESTINATION ${CMAKE_INSTALL_BINDIR})

luastatus_add_man_page (README.rst luastatus-barlib-lemonbar 7)
