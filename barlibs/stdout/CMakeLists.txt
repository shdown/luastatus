file (GLOB sources "*.c")
luastatus_add_barlib (barlib-stdout $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (barlib-stdout PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (barlib-stdout LUA)
target_include_directories (barlib-stdout PUBLIC "${PROJECT_SOURCE_DIR}")

include (GNUInstallDirs)

install (PROGRAMS luastatus-stdout-wrapper DESTINATION ${CMAKE_INSTALL_BINDIR})
