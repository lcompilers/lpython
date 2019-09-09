set(SRC
    lfort_intrinsics.c
)
add_library(lfortran_runtime SHARED ${SRC})
add_library(lfortran_runtime_static STATIC ${SRC})
install(TARGETS lfortran_runtime lfortran_runtime_static
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION share/lfortran/lib
    LIBRARY DESTINATION share/lfortran/lib
)
