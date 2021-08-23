set(SRC
    ../impure/lfortran_intrinsics.c
)
add_library(lfortran_runtime SHARED ${SRC})
set_target_properties(lfortran_runtime PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ..)
add_library(lfortran_runtime_static STATIC ${SRC})
set_target_properties(lfortran_runtime_static PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS lfortran_runtime lfortran_runtime_static
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION share/lfortran/lib
    LIBRARY DESTINATION share/lfortran/lib
)
