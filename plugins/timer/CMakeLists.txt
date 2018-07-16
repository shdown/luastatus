file (GLOB sources "*.c")
luastatus_add_plugin (plugin-timer $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (plugin-timer PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-timer LUA)
target_include_directories (plugin-timer PUBLIC "${PROJECT_SOURCE_DIR}")
