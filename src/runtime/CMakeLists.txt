set(SRC
    lfortran_intrinsics.c
)
add_library(lfortran_runtime SHARED ${SRC})
add_library(lfortran_runtime_static STATIC ${SRC})
install(TARGETS lfortran_runtime lfortran_runtime_static
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION share/lfortran/lib
    LIBRARY DESTINATION share/lfortran/lib
)

set(runtime_components
    lfortran_runtime_pure
    lfortran_runtime_impure
    lfortran_runtime_builtin
    )

add_library(lfortran_unif_runtime INTERFACE)
target_link_libraries(lfortran_unif_runtime INTERFACE ${runtime_components})

# Install
install(TARGETS lfortran_unif_runtime)
