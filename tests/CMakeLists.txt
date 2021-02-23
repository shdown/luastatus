luastatus_add_plugin_noinstall (plugin-mock $<TARGET_OBJECTS:ls> $<TARGET_OBJECTS:moonvisit> "mock_plugin.c")
target_compile_definitions (plugin-mock PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-mock LUA)
target_include_directories (plugin-mock PUBLIC "${PROJECT_SOURCE_DIR}")

luastatus_add_barlib_noinstall (barlib-mock $<TARGET_OBJECTS:ls> "mock_barlib.c")
target_compile_definitions (barlib-mock PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (barlib-mock LUA)
target_include_directories (barlib-mock PUBLIC "${PROJECT_SOURCE_DIR}")

add_executable (parrot "parrot.c")
target_compile_definitions (parrot PUBLIC -D_POSIX_C_SOURCE=200809L)
# find pthreads
set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
set (THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package (Threads REQUIRED)
# link against pthreads
target_link_libraries (parrot PUBLIC Threads::Threads)

add_executable (stopwatch "stopwatch.c")
target_compile_definitions (stopwatch PUBLIC -D_POSIX_C_SOURCE=200809L)

if (BUILD_PLUGIN_INOTIFY)
    add_executable (listnets "listnets.c")
    target_compile_definitions (listnets PUBLIC -D_POSIX_C_SOURCE=200809L)
endif ()
