project(tests)

macro(ADDTEST name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} lpython_lib  ${ARGN})
    add_test(${name} ${PROJECT_BINARY_DIR}/${name})
endmacro(ADDTEST)

macro(ADDTESTC name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} lpython_lib  ${ARGN})
    add_test(${name} ${PROJECT_BINARY_DIR}/${name})
endmacro(ADDTESTC)

ADDTEST(test_stacktrace)

set(SRC
    test_parse.cpp
    test_stacktrace2.cpp
    test_asm.cpp
    test_serialization.cpp
    test_error_rendering.cpp
)

if (WITH_LLVM)
    set(SRC ${SRC}
        test_llvm.cpp
    )
endif()


# Add one main test suite for LPython, composed of many individual cpp files:
add_executable(test_lpython ${SRC})
target_link_libraries(test_lpython lpython_lib p::doctest)
add_test(test_lpython ${PROJECT_BINARY_DIR}/test_lpython)

set_target_properties(test_lpython PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>)

if (WITH_LLVM)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options(test_lpython PRIVATE "LINKER:--export-dynamic")
    endif()
endif()
