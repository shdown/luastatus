file (GLOB sources "*.c")
luastatus_add_plugin (timer $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (timer PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (timer LUA)
target_include_directories (timer PUBLIC "${PROJECT_SOURCE_DIR}")
