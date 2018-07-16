luastatus_add_plugin_noinstall (plugin-mock $<TARGET_OBJECTS:ls> "mock_plugin.c")
target_compile_definitions (plugin-mock PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-mock LUA)
target_include_directories (plugin-mock PUBLIC "${PROJECT_SOURCE_DIR}")

luastatus_add_barlib_noinstall (barlib-mock $<TARGET_OBJECTS:ls> "mock_barlib.c")
target_compile_definitions (barlib-mock PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (barlib-mock LUA)
target_include_directories (barlib-mock PUBLIC "${PROJECT_SOURCE_DIR}")
