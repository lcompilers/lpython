if (WITH_LLVM)
    add_executable(lfortran lfortran.cpp)
    target_link_libraries(lfortran lfortran_lib)
endif()

add_executable(parse parse.cpp)
target_link_libraries(parse lfortran_lib)

add_executable(parse2 parse2.cpp)
target_link_libraries(parse2 lfortran_lib)
