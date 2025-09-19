file (GLOB sources "*.c")
add_executable (httpserv ${sources})
target_compile_definitions (httpserv PUBLIC -D_POSIX_C_SOURCE=200809L)

find_package (PkgConfig REQUIRED)
pkg_check_modules (LWS REQUIRED libwebsockets)

target_include_directories (httpserv SYSTEM PUBLIC ${LWS_INCLUDE_DIRS})
target_compile_options (httpserv PUBLIC ${LWS_CFLAGS_OTHER})
target_link_libraries (httpserv PUBLIC ${LWS_LIBRARIES})
