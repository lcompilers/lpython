project(tests)

macro(ADDTEST name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} lfortran_lib  ${ARGN})
    add_test(${name} ${PROJECT_BINARY_DIR}/${name})
endmacro(ADDTEST)

macro(ADDTESTC name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} lfortran_lib  ${ARGN})
    add_test(${name} ${PROJECT_BINARY_DIR}/${name})
endmacro(ADDTESTC)

# Copies file from source to binary directory for out-of-tree builds
macro(COPYFILE filename)
    if (NOT(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR}))
        add_custom_target(copy-${filename} ALL DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/${filename})
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${filename}
            COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/${filename}
                ${CMAKE_CURRENT_BINARY_DIR}/${filename}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${filename})
    endif()
endmacro(COPYFILE)


ADDTESTC(test_cwrapper)
ADDTEST(test_stacktrace)
COPYFILE(ref_pickle.txt)

set(SRC
    test_parse.cpp
    test_ast.cpp
    test_stacktrace2.cpp
    test_asm.cpp
    test_serialization.cpp
    test_pickle.cpp
)

if (WITH_JSON)
    set(SRC ${SRC}
        test_ast_to_json.cpp
    )
endif()
if (WITH_LLVM)
    set(SRC ${SRC}
        test_llvm.cpp
    )
endif()


# Add one main test suite for LFortran, composed of many individual cpp files:
add_executable(test_lfortran ${SRC})
target_link_libraries(test_lfortran lfortran_lib p::doctest)
add_test(test_lfortran ${PROJECT_BINARY_DIR}/test_lfortran)

if (WITH_LLVM)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options(test_lfortran PRIVATE "LINKER:--export-dynamic")
    endif()
endif()
