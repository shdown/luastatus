file (GLOB sources "*.c")
add_library (runshell OBJECT ${sources})

target_compile_definitions (runshell PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (runshell LUA)

set_target_properties (runshell PROPERTIES POSITION_INDEPENDENT_CODE ON)
