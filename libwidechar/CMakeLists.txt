file (GLOB sources "*.c")
add_library (widechar OBJECT ${sources})

include (CheckSymbolExists)
set (CMAKE_REQUIRED_DEFINITIONS "-D_XOPEN_SOURCE=500")
check_symbol_exists (wcwidth "wchar.h" LUASTATUS_HAVE_WCWIDTH)
configure_file ("config.in.h" "config.generated.h")

target_compile_definitions (widechar PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (widechar LUA)

set_target_properties (widechar PROPERTIES POSITION_INDEPENDENT_CODE ON)
