file (GLOB sources "*.c")
add_library (moonvisit OBJECT ${sources})

target_compile_definitions (moonvisit PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (moonvisit LUA)
target_include_directories (moonvisit PUBLIC "${PROJECT_SOURCE_DIR}")

set_target_properties (moonvisit PROPERTIES POSITION_INDEPENDENT_CODE ON)
