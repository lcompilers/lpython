add_executable(lfortran lfortran.cpp tpl/whereami/whereami.cpp)
target_link_libraries(lfortran lfortran_lib)
if (UNIX AND NOT APPLE)
    # This is sometimes needed to fix link errors for CLI11
    target_link_libraries(lfortran stdc++fs)
endif()
if (LFORTRAN_STATIC_BIN)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Link statically on Linux with gcc or clang
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
                CMAKE_CXX_COMPILER_ID MATCHES Clang)
            target_link_options(lfortran PRIVATE -static)
        endif()
    endif()
endif()

# Ensure "Release" is not appended to the path on Windows:
# https://stackoverflow.com/a/56201564/479532
set_target_properties(lfortran PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<0:>)

install(TARGETS lfortran
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION share/lfortran/lib
    LIBRARY DESTINATION share/lfortran/lib
)


add_executable(parse parse.cpp)
target_link_libraries(parse lfortran_lib)

add_executable(parse2 parse2.cpp)
target_link_libraries(parse2 lfortran_lib)

add_executable(cpptranslate cpptranslate.cpp)
target_link_libraries(cpptranslate lfortran_lib)
if (UNIX AND NOT APPLE)
    # This is sometimes needed to fix link errors for CLI11
    target_link_libraries(cpptranslate stdc++fs)
endif()
