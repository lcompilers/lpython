set(SRC
    ../impure/lfortran_intrinsics.c
)
add_library(lfortran_runtime SHARED ${SRC})
set_target_properties(lfortran_runtime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>)
add_library(lfortran_runtime_static STATIC ${SRC})
set_target_properties(lfortran_runtime_static PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>)
install(TARGETS lfortran_runtime lfortran_runtime_static
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION share/lpython/lib
    LIBRARY DESTINATION share/lpython/lib
)
