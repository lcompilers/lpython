set(SRC
    ../../../src/libasr/src/libasr/runtime/lfortran_intrinsics.c
)
add_library(lpython_runtime SHARED ${SRC})
target_include_directories(lpython_runtime BEFORE PUBLIC ${libasr/src/libasr_SOURCE_DIR}/..)
target_include_directories(lpython_runtime BEFORE PUBLIC ${libasr/src/libasr_BINARY_DIR}/..)
set_target_properties(lpython_runtime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>)
add_library(lpython_runtime_static STATIC ${SRC})
target_include_directories(lpython_runtime_static BEFORE PUBLIC ${libasr/src/libasr_SOURCE_DIR}/..)
target_include_directories(lpython_runtime_static BEFORE PUBLIC ${libasr/src/libasr_BINARY_DIR}/..)
set_target_properties(lpython_runtime_static PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../$<0:>)
install(TARGETS lpython_runtime lpython_runtime_static
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION share/lpython/lib
    LIBRARY DESTINATION share/lpython/lib
)
