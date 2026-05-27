file (GLOB sources "*.c")
add_library (lreplace OBJECT ${sources})

target_compile_definitions (lreplace PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (lreplace LUA)

find_library (MATH_LIBRARY m)
if (MATH_LIBRARY)
    target_link_libraries (lreplace PUBLIC ${MATH_LIBRARY})
endif ()

set_target_properties (lreplace PROPERTIES POSITION_INDEPENDENT_CODE ON)
