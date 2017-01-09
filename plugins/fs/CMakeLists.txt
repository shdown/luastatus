file (GLOB sources "*.c")
luastatus_add_plugin (fs $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (fs PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (fs LUA)
target_include_directories (fs PUBLIC "${PROJECT_SOURCE_DIR}")
