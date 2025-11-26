file (GLOB sources "*.c")
add_library (safe OBJECT ${sources})

target_compile_definitions (safe PUBLIC -D_POSIX_C_SOURCE=200809L)

set_target_properties (safe PROPERTIES POSITION_INDEPENDENT_CODE ON)
