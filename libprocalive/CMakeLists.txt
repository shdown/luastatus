file (GLOB sources "*.c")
add_library (procalive OBJECT ${sources})

target_compile_definitions (procalive PUBLIC -D_POSIX_C_SOURCE=200809L)
luastatus_target_compile_with (procalive LUA)

find_library (MATH_LIBRARY m)
if (MATH_LIBRARY)
    target_link_libraries (procalive PUBLIC ${MATH_LIBRARY})
endif ()

set_target_properties (procalive PROPERTIES POSITION_INDEPENDENT_CODE ON)
