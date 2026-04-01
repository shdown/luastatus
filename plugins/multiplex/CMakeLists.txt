file (GLOB sources "*.c")
luastatus_add_plugin (
    plugin-multiplex
    $<TARGET_OBJECTS:ls>
    $<TARGET_OBJECTS:moonvisit>
    $<TARGET_OBJECTS:runshell>
    $<TARGET_OBJECTS:widechar>
    $<TARGET_OBJECTS:safe>
    ${sources}
)

configure_file ("config.in.h" "config.generated.h")

target_compile_definitions (plugin-multiplex PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (plugin-multiplex LUA)
target_include_directories (plugin-multiplex PUBLIC "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

# find pthreads
set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
set (THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package (Threads REQUIRED)
# link against dl and pthread
target_link_libraries (plugin-multiplex PUBLIC ${CMAKE_DL_LIBS} Threads::Threads)

luastatus_add_man_page (README.rst luastatus-plugin-multiplex 7)
