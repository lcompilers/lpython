add_executable(lpython lpython.cpp)
target_include_directories(lpython PRIVATE "tpl")
target_link_libraries(lpython lfortran_lib)
if (LFORTRAN_STATIC_BIN)
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


if (WITH_RUNTIME_LIBRARY)
    macro(LFORTRAN_COMPILE_RUNTIME name dir)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../runtime/${name}.mod
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/lpython
            ARGS --backend=cpp -c ${CMAKE_CURRENT_SOURCE_DIR}/../runtime/${dir}/${name}.f90 -o ${name}.o
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../runtime
            DEPENDS lpython ${CMAKE_CURRENT_SOURCE_DIR}/../runtime/${dir}/${name}.f90 ${ARGN}
            COMMENT "LFortran Compiling ${dir}/${name}.f90")
    endmacro(LFORTRAN_COMPILE_RUNTIME)

    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_builtin builtin)
    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_kind pure)
    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_iso_fortran_env pure)
    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_ieee_arithmetic pure
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_fortran_env.mod
    )
    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_iso_c_binding pure
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_builtin.mod
    )
    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_string pure
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_builtin.mod
    )
    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_math2 pure
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_fortran_env.mod
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_builtin.mod
    )
    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_trig pure
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_fortran_env.mod
    )
    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_math impure
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_c_binding.mod
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_builtin.mod
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_fortran_env.mod
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_math2.mod
    )
    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_sin impure
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_c_binding.mod
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_builtin.mod
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_fortran_env.mod
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_math.mod
    )
    LFORTRAN_COMPILE_RUNTIME(lfortran_intrinsic_bit impure
      ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_fortran_env.mod
    )

    add_custom_target(lfortran2
        ALL
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_builtin.mod
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_kind.mod
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_fortran_env.mod
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_ieee_arithmetic.mod
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_c_binding.mod
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_math2.mod
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_math.mod
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_trig.mod
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_sin.mod
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_string.mod
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_bit.mod
    )

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_builtin.mod
              ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_kind.mod
              ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_fortran_env.mod
              ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_ieee_arithmetic.mod
              ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_iso_c_binding.mod
              ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_math2.mod
              ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_math.mod
              ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_trig.mod
              ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_sin.mod
              ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_string.mod
              ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lfortran_intrinsic_bit.mod
        DESTINATION share/lpython/lib
    )
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
