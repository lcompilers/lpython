add_executable(lfortran lfortran.cpp)
target_include_directories(lfortran PRIVATE "tpl")
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
if (WITH_STACKTRACE AND APPLE AND CMAKE_CXX_COMPILER_ID MATCHES Clang)
    # On macOS we have to call dsymutil to create the dSYM bundle so that the
    # stacktrace can find debugging information corresponding to the lfortran
    # binary
    add_custom_command(
        TARGET lfortran
        POST_BUILD
        COMMAND dsymutil lfortran
    )
    if (WITH_DWARFDUMP)
        add_custom_command(
            TARGET lfortran
            POST_BUILD
            COMMAND llvm-dwarfdump --debug-line lfortran.dSYM > lfortran.dSYM/raw.txt
        )
        add_custom_command(
            TARGET lfortran
            POST_BUILD
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/dwarf_convert.py lfortran.dSYM/raw.txt lfortran.dSYM/lines.txt lfortran.dSYM/lines.dat
        )
    endif()
endif()

add_custom_command(
    TARGET lfortran
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../runtime
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/lfortran --backend=cpp -c ${CMAKE_CURRENT_SOURCE_DIR}/../runtime/lfortran_intrinsic_kind.f90 -o lfortran_intrinsic_kind.o
)

# Ensure "Release" is not appended to the path on Windows:
# https://stackoverflow.com/a/56201564/479532
set_target_properties(lfortran PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<0:>)

set_target_properties(lfortran PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(TARGETS lfortran
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION share/lfortran/lib
    LIBRARY DESTINATION share/lfortran/lib
)


add_executable(parse parse.cpp)
target_link_libraries(parse lfortran_lib)

add_executable(parse2 parse2.cpp)
target_link_libraries(parse2 lfortran_lib)

if (WITH_FMT)
    add_executable(parse3 parse3.cpp)
    target_link_libraries(parse3 lfortran_lib fmt::fmt)
endif()

add_executable(cpptranslate cpptranslate.cpp)
target_link_libraries(cpptranslate lfortran_lib)
if (UNIX AND NOT APPLE)
    # This is sometimes needed to fix link errors for CLI11
    target_link_libraries(cpptranslate stdc++fs)
endif()
