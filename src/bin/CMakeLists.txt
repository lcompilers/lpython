add_executable(lpython lpython.cpp)
target_include_directories(lpython PRIVATE "tpl")
target_link_libraries(lpython lpython_lib)
if (LPYTHON_STATIC_BIN)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Link statically on Linux with gcc or clang
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
                CMAKE_CXX_COMPILER_ID MATCHES Clang)
            target_link_options(lpython PRIVATE -static)
        endif()
    endif()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(lpython PRIVATE "LINKER:--export-dynamic")
endif()

if (WITH_STACKTRACE AND APPLE AND CMAKE_CXX_COMPILER_ID MATCHES Clang)
    # On macOS we have to call dsymutil to create the dSYM bundle so that the
    # stacktrace can find debugging information corresponding to the lpython
    # binary
    add_custom_command(
        TARGET lpython
        POST_BUILD
        COMMAND dsymutil lpython
    )
    if (WITH_DWARFDUMP)
        add_custom_command(
            TARGET lpython
            POST_BUILD
            COMMAND llvm-dwarfdump --debug-line lpython.dSYM > lpython.dSYM/raw.txt
        )
        add_custom_command(
            TARGET lpython
            POST_BUILD
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/dwarf_convert.py lpython.dSYM/raw.txt lpython.dSYM/lines.txt lpython.dSYM/lines.dat
        )
    endif()
endif()


# Ensure "Release" is not appended to the path on Windows:
# https://stackoverflow.com/a/56201564/479532
set_target_properties(lpython PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<0:>)

set_target_properties(lpython PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(TARGETS lpython
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION share/lpython/lib
    LIBRARY DESTINATION share/lpython/lib
)

set_target_properties(lpython PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>)

if (WITH_INTRINSIC_MODULES)
    macro(LPYTHON_COMPILE_MODULE name)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../runtime/${name}.pyc
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/lpython
            ARGS --disable-main -c ${CMAKE_CURRENT_SOURCE_DIR}/../runtime/${name}.py -o ${name}.o
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../runtime
            DEPENDS lpython ${CMAKE_CURRENT_SOURCE_DIR}/../runtime/${name}.py ${ARGN}
            COMMENT "LPython Compiling ${name}.py")
    endmacro(LPYTHON_COMPILE_MODULE)

    LPYTHON_COMPILE_MODULE(lpython_intrinsic_numpy)
    LPYTHON_COMPILE_MODULE(lpython_builtin)

    add_custom_target(lpython_intrinsics
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lpython_intrinsic_numpy.pyc
        ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lpython_builtin.pyc
    )

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lpython_intrinsic_numpy.pyc
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lpython_builtin.pyc
        DESTINATION share/lfortran/lib
    )
endif()
