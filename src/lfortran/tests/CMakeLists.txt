project(tests)

macro(ADDTEST name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} lfortran_lib  ${ARGN})
    add_test(${name} ${PROJECT_BINARY_DIR}/${name})
endmacro(ADDTEST)

ADDTEST(test_parse p::doctest)
ADDTEST(test_ast)
ADDTEST(test_ast_to_json p::doctest)
