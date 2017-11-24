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
RUN(program_cmake_02)

RUN(expr_01 fail)
RUN(expr_02)
RUN(expr_03)

RUN(abort_01 fail)

RUN(if_01)
RUN(if_02 fail)
RUN(if_03 fail)

RUN(cond_01)

RUN(print_01)

RUN(variables_01)
RUN(variables_02)
RUN(variables_03)

RUN(while_01)
RUN(while_02)
RUN(doloop_01)
RUN(doloop_02)
RUN(doloop_03)

RUN(arrays_01)

RUN(subroutines_01)
