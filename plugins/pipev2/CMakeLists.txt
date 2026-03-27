file (GLOB sources "*.c")
luastatus_add_plugin (
    plugin-pipev2
    $<TARGET_OBJECTS:ls>
    $<TARGET_OBJECTS:moonvisit>
    ${sources}
)

target_compile_definitions (plugin-pipev2 PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-pipev2 LUA)
target_include_directories (plugin-pipev2 PUBLIC "${PROJECT_SOURCE_DIR}")

# find pthreads
set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
set (THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package (Threads REQUIRED)
# link aginst pthread
target_link_libraries (plugin-pipev2 PUBLIC Threads::Threads)

luastatus_add_man_page (README.rst luastatus-plugin-pipev2 7)
