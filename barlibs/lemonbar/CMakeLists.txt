file (GLOB sources "*.c")
luastatus_add_barlib (lemonbar $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (lemonbar PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (lemonbar LUA)
target_include_directories (lemonbar PUBLIC "${PROJECT_SOURCE_DIR}")

include(GNUInstallDirs)

install (PROGRAMS luastatus-lemonbar-launcher DESTINATION ${CMAKE_INSTALL_BINDIR})
