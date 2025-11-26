file (GLOB sources "*.c")
add_library (moonvisit OBJECT ${sources})

target_compile_definitions (moonvisit PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (moonvisit LUA)

find_library (MATH_LIBRARY m)
if (MATH_LIBRARY)
    target_link_libraries (moonvisit PUBLIC ${MATH_LIBRARY})
endif ()

set_target_properties (moonvisit PROPERTIES POSITION_INDEPENDENT_CODE ON)
