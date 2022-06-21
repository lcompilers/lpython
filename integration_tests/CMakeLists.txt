cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(lpython_tests C)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug
        CACHE STRING "Build type (Debug, Release)" FORCE)
endif ()
if (NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" OR
        CMAKE_BUILD_TYPE STREQUAL "Release"))
    message("${CMAKE_BUILD_TYPE}")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be one of: Debug, Release (current value: '${CMAKE_BUILD_TYPE}')")
endif ()

set(KIND no CACHE STRING "Type of Test")

find_path(LPYTHON_RTLIB_DIR lfortran_intrinsics.h
    ${CMAKE_SOURCE_DIR}/../src/runtime/impure)
find_library(LPYTHON_RTLIB_LIBRARY lpython_runtime_static
    ${CMAKE_SOURCE_DIR}/../src/runtime/)
add_library(lpython_rtlib INTERFACE IMPORTED)
set_property(TARGET lpython_rtlib PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${LPYTHON_RTLIB_DIR})
set_property(TARGET lpython_rtlib PROPERTY INTERFACE_LINK_LIBRARIES
    ${LPYTHON_RTLIB_LIBRARY})
target_link_libraries(lpython_rtlib INTERFACE m)

enable_testing()

message("\n")
message("Configuration results")
message("---------------------")
message("C compiler      : ${CMAKE_C_COMPILER}")
message("Build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("C compiler flags      : ${CMAKE_C_FLAGS_DEBUG}")
else ()
    message("C compiler flags      : ${CMAKE_C_FLAGS_RELEASE}")
endif ()
message("Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message("KIND: ${KIND}")
message("LPYTHON_RTLIB_DIR: ${LPYTHON_RTLIB_DIR}")
message("LPYTHON_RTLIB_LIBRARY: ${LPYTHON_RTLIB_LIBRARY}")



macro(RUN)
    set(options FAIL)
    set(oneValueArgs NAME)
    set(multiValueArgs LABELS EXTRAFILES)
    cmake_parse_arguments(RUN "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )
    set(name ${RUN_NAME})
    if (NOT name)
        message(FATAL_ERROR "Must specify the NAME argument")
    endif()

    if (${KIND} IN_LIST RUN_LABELS)
        if (KIND STREQUAL "llvm")
            add_custom_command(
                OUTPUT ${name}.o
                COMMAND lpython -c ${name}.py -o ${name}.o
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py
                VERBATIM)
            add_executable(${name} ${name}.o ${RUN_EXTRAFILES})
            set_target_properties(${name} PROPERTIES LINKER_LANGUAGE C)
            target_link_libraries(${name} lpython_rtlib)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
            if (RUN_LABELS)
                set_tests_properties(${name} PROPERTIES LABELS "${RUN_LABELS}")
            endif()
            if (${RUN_FAIL})
               set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "c")
            add_custom_command(
                OUTPUT ${name}.c
                COMMAND lpython --show-c ${name}.py > ${name}.c
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py
                VERBATIM)
            add_executable(${name} ${name}.c ${RUN_EXTRAFILES})
            set_target_properties(${name} PROPERTIES LINKER_LANGUAGE C)
            target_link_libraries(${name} lpython_rtlib)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
            if (RUN_LABELS)
                set_tests_properties(${name} PROPERTIES LABELS "${RUN_LABELS}")
            endif()
            if (${RUN_FAIL})
               set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "cpython")
            # CPython test
            if (RUN_EXTRAFILES)
                set(PY_MOD "${name}_mod")
                add_library(${PY_MOD} SHARED ${RUN_EXTRAFILES})
                set_target_properties(${PY_MOD} PROPERTIES LINKER_LANGUAGE C)
            else()
                set(PY_MOD "")
            endif()

            add_test(${name} python ${CMAKE_CURRENT_BINARY_DIR}/${name}.py)
            set_tests_properties(${name} PROPERTIES
                ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}/../src/runtime/ltypes;LPYTHON_PY_MOD_NAME=${PY_MOD};LPYTHON_PY_MOD_PATH=${CMAKE_CURRENT_BINARY_DIR}")
            if (RUN_LABELS)
                set_tests_properties(${name} PROPERTIES LABELS "${RUN_LABELS}")
            endif()
            if (${RUN_FAIL})
               set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        endif()
    endif()
endmacro(RUN)


# Test zero and non-zero exit code and assert statements
RUN(NAME bindc_01            LABELS llvm c)
RUN(NAME bindc_02            LABELS llvm c)
RUN(NAME bindc_04            LABELS llvm)
RUN(NAME exit_01             LABELS cpython llvm c)
RUN(NAME exit_02     FAIL    LABELS cpython llvm c)
RUN(NAME exit_01b            LABELS cpython llvm c)
RUN(NAME exit_02b    FAIL    LABELS cpython llvm c)
RUN(NAME exit_02c    FAIL    LABELS cpython llvm c)

# Test all three backends
RUN(NAME print_01            LABELS cpython llvm c)

# CPython and LLVM
RUN(NAME expr_01             LABELS cpython llvm c)
RUN(NAME expr_02             LABELS cpython llvm c)
RUN(NAME expr_03             LABELS cpython llvm c)
RUN(NAME expr_04             LABELS cpython llvm c)
RUN(NAME expr_05             LABELS cpython llvm)
RUN(NAME test_types_01       LABELS cpython llvm)
RUN(NAME test_str_01         LABELS cpython llvm)
RUN(NAME test_str_02         LABELS cpython llvm)
RUN(NAME test_list_01        LABELS cpython llvm)
RUN(NAME modules_01          LABELS cpython llvm)
RUN(NAME test_math           LABELS cpython llvm)
RUN(NAME test_numpy_01       LABELS cpython llvm)
RUN(NAME test_numpy_02       LABELS cpython llvm)
RUN(NAME test_random         LABELS cpython llvm)
RUN(NAME test_os             LABELS cpython llvm)
RUN(NAME test_builtin        LABELS cpython llvm)
RUN(NAME test_builtin_abs    LABELS cpython llvm)
RUN(NAME test_builtin_bool   LABELS cpython llvm)
RUN(NAME test_builtin_pow    LABELS cpython llvm)
RUN(NAME test_builtin_int    LABELS cpython llvm)
RUN(NAME test_builtin_len    LABELS cpython llvm)
RUN(NAME test_builtin_float  LABELS cpython llvm)
RUN(NAME test_builtin_str_02 LABELS cpython llvm)
RUN(NAME test_builtin_round  LABELS cpython llvm)
RUN(NAME test_math1          LABELS cpython llvm)
RUN(NAME test_math_02        LABELS cpython llvm)
RUN(NAME test_c_interop_01   LABELS cpython llvm c)
RUN(NAME test_c_interop_02   LABELS cpython llvm c
        EXTRAFILES test_c_interop_02b.c)
RUN(NAME test_c_interop_03   LABELS cpython llvm c
        EXTRAFILES test_c_interop_03b.c)
RUN(NAME test_c_interop_04   LABELS cpython llvm c
        EXTRAFILES test_c_interop_04b.c)
RUN(NAME test_c_interop_05   LABELS llvm c
        EXTRAFILES test_c_interop_05b.c)
RUN(NAME bindc_03   LABELS llvm
        EXTRAFILES bindc_03b.c)
RUN(NAME test_generics_01    LABELS cpython llvm)
RUN(NAME test_cmath          LABELS cpython llvm)
RUN(NAME test_complex        LABELS cpython llvm)
RUN(NAME test_max_min        LABELS cpython llvm)
RUN(NAME test_integer_bitnot LABELS cpython llvm)
RUN(NAME test_unary_minus    LABELS cpython llvm)
RUN(NAME test_unary_plus     LABELS cpython llvm)
RUN(NAME test_bool_binop     LABELS cpython llvm)
RUN(NAME test_issue_518      LABELS cpython llvm)
RUN(NAME structs_01          LABELS cpython llvm c)
RUN(NAME structs_02          LABELS llvm c)
RUN(NAME structs_03          LABELS llvm c)
RUN(NAME test_str_to_int     LABELS cpython llvm)
RUN(NAME test_platform       LABELS cpython llvm)
RUN(NAME test_vars_01        LABELS cpython llvm)
RUN(NAME test_version        LABELS cpython llvm)

# Just CPython
RUN(NAME test_builtin_bin    LABELS cpython)
