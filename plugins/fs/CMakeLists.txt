file (GLOB sources "*.c")
luastatus_add_plugin (plugin-fs $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (plugin-fs PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-fs LUA)
target_include_directories (plugin-fs PUBLIC "${PROJECT_SOURCE_DIR}")
