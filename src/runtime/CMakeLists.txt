set(SRC
    lfort_intrinsics.c
)
add_library(lfortran SHARED ${SRC})
add_library(lfortran_static STATIC ${SRC})
install(TARGETS lfortran lfortran_static
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION share/lfortran/lib
    LIBRARY DESTINATION share/lfortran/lib
)
