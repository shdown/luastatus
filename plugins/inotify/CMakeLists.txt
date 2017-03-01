file (GLOB sources "*.c")
luastatus_add_plugin (inotify $<TARGET_OBJECTS:ls> ${sources})

include (CheckSymbolExists)
check_symbol_exists (inotify_init1 "sys/inotify.h" HAVE_INOTIFY_INIT1)
configure_file ("probes.in.h" "probes.generated.h")

target_compile_definitions (inotify PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (inotify LUA)
target_include_directories (inotify PUBLIC "${PROJECT_SOURCE_DIR}")
