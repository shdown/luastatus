file (GLOB sources "*.c")
luastatus_add_plugin (plugin-ipaddr $<TARGET_OBJECTS:ls> ${sources})

target_compile_definitions (plugin-ipaddr PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-ipaddr LUA)
target_include_directories (plugin-ipaddr PUBLIC "${PROJECT_SOURCE_DIR}")

luastatus_add_man_page (README.rst luastatus-plugin-ipaddr 7)
