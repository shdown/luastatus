file (GLOB sources "*.c")
add_library (lrand OBJECT ${sources})

target_compile_definitions (lrand PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (lrand LUA)

find_library (MATH_LIBRARY m)
if (MATH_LIBRARY)
    target_link_libraries (lrand PUBLIC ${MATH_LIBRARY})
endif ()

set_target_properties (lrand PROPERTIES POSITION_INDEPENDENT_CODE ON)
