luastatus_add_plugin_noinstall (mock_plugin $<TARGET_OBJECTS:ls> "mock_plugin.c")
target_compile_definitions (mock_plugin PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (mock_plugin LUA)
target_include_directories (mock_plugin PUBLIC "${PROJECT_SOURCE_DIR}")

luastatus_add_barlib_noinstall (mock_barlib $<TARGET_OBJECTS:ls> "mock_barlib.c")
target_compile_definitions (mock_barlib PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (mock_barlib LUA)
target_include_directories (mock_barlib PUBLIC "${PROJECT_SOURCE_DIR}")
