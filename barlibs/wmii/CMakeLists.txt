file (GLOB sources "*.c")
luastatus_add_barlib (barlib-wmii $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (barlib-wmii PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (barlib-wmii LUA)
target_include_directories (barlib-wmii PUBLIC "${PROJECT_SOURCE_DIR}")

find_library (IXP_LIBRARY ixp)
target_link_libraries (barlib-wmii ${IXP_LIBRARY})

include (GNUInstallDirs)

install (PROGRAMS luastatus-wmii-wrapper DESTINATION ${CMAKE_INSTALL_BINDIR})
