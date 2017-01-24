set (sources "lemonbar.c")
luastatus_add_barlib (lemonbar $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (lemonbar PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (lemonbar LUA)
target_include_directories (lemonbar PUBLIC "${PROJECT_SOURCE_DIR}")

add_executable (luastatus-lemonbar-launcher $<TARGET_OBJECTS:ls> "luastatus-lemonbar-launcher.c")
target_compile_definitions (luastatus-lemonbar-launcher PUBLIC -D_POSIX_C_SOURCE=200809L)
target_include_directories (luastatus-lemonbar-launcher PUBLIC "${PROJECT_SOURCE_DIR}")

include(GNUInstallDirs)

install (TARGETS luastatus-lemonbar-launcher DESTINATION ${CMAKE_INSTALL_BINDIR})
