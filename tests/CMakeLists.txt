macro(RUN name)
    add_executable(${name} ${name}.f90)
    add_test(${name} ${CURRENT_BINARY_DIR}/${name})
endmacro(RUN)

macro(COMPILE name)
    add_executable(${name} ${name}.f90)
endmacro(COMPILE)

RUN(program_cmake_01)
RUN(expr_01)
RUN(abort_01)
set_tests_properties(abort_01 PROPERTIES WILL_FAIL TRUE)

RUN(if_01)
RUN(if_02)
set_tests_properties(if_02 PROPERTIES WILL_FAIL TRUE)
RUN(if_03)
set_tests_properties(if_03 PROPERTIES WILL_FAIL TRUE)

RUN(cond_01)
