file (GLOB sources "*.c")
add_library (ls OBJECT ${sources})

include (CheckSymbolExists)
set (CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
check_symbol_exists (pipe2 "fcntl.h;unistd.h" LS_HAVE_GNU_PIPE2)
check_symbol_exists (SOCK_CLOEXEC "sys/socket.h" LS_HAVE_GNU_SOCK_CLOEXEC)
set (CMAKE_REQUIRED_DEFINITIONS "-D_POSIX_C_SOURCE=200809L")
check_symbol_exists (posix_close "unistd.h" LS_HAVE_POSIX_CLOSE)
configure_file ("probes.in.h" "probes.generated.h")

target_compile_definitions (ls PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (ls LUA)
target_include_directories (ls PUBLIC "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

set_target_properties (ls PROPERTIES POSITION_INDEPENDENT_CODE ON)
