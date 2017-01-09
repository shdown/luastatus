file (GLOB sources "*.c")
luastatus_add_plugin (pipe $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (pipe PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (pipe LUA)
target_include_directories (pipe PUBLIC "${PROJECT_SOURCE_DIR}")
