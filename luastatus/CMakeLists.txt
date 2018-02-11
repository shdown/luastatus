if (NOT luastatus_POSIX_SH_PATH)
    set (luastatus_POSIX_SH_PATH "/bin/sh")
endif ()

execute_process (
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/version.sh" "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE luastatus_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file ("config.in.h" "config.generated.h")
add_executable (luastatus $<TARGET_OBJECTS:ls> "luastatus.c")

target_compile_definitions (luastatus PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_build_with (luastatus LUA)
target_include_directories (luastatus PUBLIC "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries (luastatus dl pthread)

include (GNUInstallDirs)

install (TARGETS luastatus DESTINATION ${CMAKE_INSTALL_BINDIR})
install (FILES luastatus.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
