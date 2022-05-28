cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(lcompilers_unif_runtime C Fortran)

add_subdirectory(builtin)
add_subdirectory(pure)
add_subdirectory(impure)

set(runtime_components
    lcompilers_runtime_pure
    lcompilers_runtime_impure
    lcompilers_runtime_builtin
    )

add_library(lcompilers_unif_runtime INTERFACE)
target_link_libraries(lcompilers_unif_runtime INTERFACE ${runtime_components})

# Install
install(TARGETS lcompilers_unif_runtime)
