Python_add_library(cparser MODULE cparser.c)
target_link_libraries(cparser PRIVATE lfortran_lib)
