file (GLOB sources "*.c")
add_library (hackyfix OBJECT ${sources})

target_compile_definitions (hackyfix PUBLIC -D_POSIX_C_SOURCE=200809L)

# link against dl
target_link_libraries (hackyfix PUBLIC ${CMAKE_DL_LIBS})

set_target_properties (ls PROPERTIES POSITION_INDEPENDENT_CODE ON)
