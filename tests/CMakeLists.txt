macro(ADDTEST name)
    add_executable(${name} ${name}.f90)
    add_test(${name} ${PROJECT_BINARY_DIR}/${name})
endmacro(ADDTEST)

macro(ADDPROG name)
    add_executable(${name} ${name}.f90)
endmacro(ADDPROG)

project(tests)

ADDTEST(test_program_cmake_01)
ADDTEST(test_expr_01)
