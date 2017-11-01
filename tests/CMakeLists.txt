macro(RUN name)
    add_executable(${name} ${name}.f90)
    add_test(${name} ${CURRENT_BINARY_DIR}/${name})
    set(extra_args ${ARGN})
    if (extra_args STREQUAL "fail")
        set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
    endif()
endmacro(RUN)

macro(COMPILE name)
    add_executable(${name} ${name}.f90)
endmacro(COMPILE)

RUN(program_cmake_01)

RUN(expr_01)
RUN(expr_02)
RUN(expr_03 fail)

RUN(abort_01 fail)

RUN(if_01)
RUN(if_02 fail)
RUN(if_03 fail)

RUN(cond_01)
