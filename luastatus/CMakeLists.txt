if (NOT luastatus_POSIX_SH_PATH)
    set (luastatus_POSIX_SH_PATH "/bin/sh")
endif ()
configure_file ("config.in.h" "config.generated.h")

file (GLOB sources "*.c")
add_executable (luastatus $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (luastatus PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_build_with (luastatus LUA)
target_include_directories (luastatus PUBLIC "${PROJECT_SOURCE_DIR}")
target_link_libraries (luastatus dl pthread)

add_custom_target (version
    ./version_h_generate.sh "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM)
add_dependencies (luastatus version)

include(GNUInstallDirs)

install (TARGETS luastatus DESTINATION ${CMAKE_INSTALL_BINDIR})
install (FILES luastatus.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
