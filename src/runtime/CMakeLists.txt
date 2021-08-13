set(SRC
    impure/lfortran_intrinsics.c
)
add_library(lfortran_runtime SHARED ${SRC})
add_library(lfortran_runtime_static STATIC ${SRC})
install(TARGETS lfortran_runtime lfortran_runtime_static
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION share/lfortran/lib
    LIBRARY DESTINATION share/lfortran/lib
)
