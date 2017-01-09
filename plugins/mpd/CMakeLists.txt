file (GLOB sources "*.c")
luastatus_add_plugin (mpd $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (mpd PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (mpd LUA)
target_include_directories (mpd PUBLIC "${PROJECT_SOURCE_DIR}")
