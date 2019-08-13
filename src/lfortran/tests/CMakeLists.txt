project(tests)

add_executable(test_parse test_parse.cpp)
target_link_libraries(test_parse lfortran_lib)
add_test(test_parse ${PROJECT_BINARY_DIR}/test_parse)
