cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(lfortran_unif_runtime C Fortran)

add_subdirectory(builtin)
add_subdirectory(pure)
add_subdirectory(impure)

set(runtime_components
    lfortran_runtime_pure
    lfortran_runtime_impure
    lfortran_runtime_builtin
    )

add_library(lfortran_unif_runtime INTERFACE)
target_link_libraries(lfortran_unif_runtime INTERFACE ${runtime_components})

# Install
install(TARGETS lfortran_unif_runtime)
