file (GLOB sources "*.c")
add_library (ls OBJECT ${sources})

include (CheckSymbolExists)
set (CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
check_symbol_exists (pipe2 "fcntl.h;unistd.h" LS_HAVE_GNU_PIPE2)
check_symbol_exists (SOCK_CLOEXEC "sys/socket.h" LS_HAVE_GNU_SOCK_CLOEXEC)
configure_file ("ls_probes.in.h" "ls_probes.generated.h")

target_compile_definitions (ls PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (ls LUA)
target_include_directories (ls PUBLIC "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

find_library (MATH_LIBRARY m)
if (MATH_LIBRARY)
    target_link_libraries (ls PUBLIC ${MATH_LIBRARY})
endif ()

set_target_properties (ls PROPERTIES POSITION_INDEPENDENT_CODE ON)
