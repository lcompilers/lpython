project(tests)

add_executable(test_parse test_parse.cpp)
target_link_libraries(test_parse lfortran_lib p::doctest)
add_test(test_parse ${PROJECT_BINARY_DIR}/test_parse)

add_executable(test_ast test_ast.cpp)
target_link_libraries(test_ast lfortran_lib)
add_test(test_ast ${PROJECT_BINARY_DIR}/test_ast)
