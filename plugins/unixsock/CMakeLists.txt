file (GLOB sources "*.c")
luastatus_add_plugin (plugin-unixsock $<TARGET_OBJECTS:ls> $<TARGET_OBJECTS:moonvisit> ${sources})

include (CheckSymbolExists)
set (CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
check_symbol_exists (accept4 "sys/socket.h" HAVE_GNU_ACCEPT4)
configure_file ("probes.in.h" "probes.generated.h")

target_compile_definitions (plugin-unixsock PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-unixsock LUA)
target_include_directories (plugin-unixsock PUBLIC "${PROJECT_SOURCE_DIR}")

luastatus_add_man_page (README.rst luastatus-plugin-unixsock 7)
