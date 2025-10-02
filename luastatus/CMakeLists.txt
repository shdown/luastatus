execute_process (
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/version.sh" "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE luastatus_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

include (CheckSymbolExists)
set (CMAKE_REQUIRED_DEFINITIONS "-D_XOPEN_SOURCE=500")
check_symbol_exists (wcwidth "wchar.h" LUASTATUS_HAVE_WCWIDTH)

configure_file ("config.in.h" "config.generated.h")

file (GLOB sources "*.c")
add_executable (luastatus $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (luastatus PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_build_with (luastatus LUA)
target_include_directories (luastatus PUBLIC "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

# find pthreads
set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
set (THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package (Threads REQUIRED)
# link against dl and pthread
target_link_libraries (luastatus PUBLIC ${CMAKE_DL_LIBS} Threads::Threads)

include (GNUInstallDirs)

install (TARGETS luastatus DESTINATION ${CMAKE_INSTALL_BINDIR})
luastatus_add_man_page (README.rst luastatus 1)
