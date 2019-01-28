file (GLOB sources "*.c")
luastatus_add_plugin (plugin-mpd $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (plugin-mpd PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-mpd LUA)
target_include_directories (plugin-mpd PUBLIC "${PROJECT_SOURCE_DIR}")

luastatus_add_man_page (README.rst luastatus-plugin-mpd 7)
