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
    ${CMAKE_SOURCE_DIR}/../src/libasr/runtime)
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
    set(oneValueArgs NAME IMPORT_PATH)
    set(multiValueArgs LABELS EXTRAFILES)
    cmake_parse_arguments(RUN "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )
    set(name ${RUN_NAME})
    set(import_path ${RUN_IMPORT_PATH})
    if (NOT name)
        message(FATAL_ERROR "Must specify the NAME argument")
    endif()

    if (${KIND} IN_LIST RUN_LABELS)
        if (KIND STREQUAL "llvm")
            if (import_path)
                add_custom_command(
                    OUTPUT ${name}.o
                    COMMAND lpython -c -I ${CMAKE_CURRENT_SOURCE_DIR}/${import_path} ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py -o ${name}.o
                    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py
                    VERBATIM)
            else ()
                add_custom_command(
                    OUTPUT ${name}.o
                    COMMAND lpython -c ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py -o ${name}.o
                    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py
                    VERBATIM)
            endif()
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
                COMMAND lpython --show-c ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py > ${name}.c
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

            add_test(${name} python ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py)
            set_tests_properties(${name} PROPERTIES
                ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}/../src/runtime/ltypes:${CMAKE_SOURCE_DIR}/..;LPYTHON_PY_MOD_NAME=${PY_MOD};LPYTHON_PY_MOD_PATH=${CMAKE_CURRENT_BINARY_DIR}")
            if (RUN_LABELS)
                set_tests_properties(${name} PROPERTIES LABELS "${RUN_LABELS}")
            endif()
            if (${RUN_FAIL})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "x86")
            # x86 test
            add_custom_command(
                OUTPUT ${name}.x86
                COMMAND lpython --backend x86 ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py -o ${name}.x86
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py
                VERBATIM
            )
            add_custom_target(${name} ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${name}.x86)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.x86)
            if (RUN_LABELS)
                set_tests_properties(${name} PROPERTIES LABELS "${RUN_LABELS}")
            endif()
            if (${RUN_FAIL})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "wasm_x86")
            # wasm_to_x86 test
            add_custom_command(
                OUTPUT ${name}.x86
                COMMAND lpython --backend wasm_x86 ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py -o ${name}.x86
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py
                VERBATIM
            )
            add_custom_target(${name} ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${name}.x86)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.x86)
            if (RUN_LABELS)
                set_tests_properties(${name} PROPERTIES LABELS "${RUN_LABELS}")
            endif()
            if (${RUN_FAIL})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "wasm_x64")
            # wasm_to_x64 test
            add_custom_command(
                OUTPUT ${name}.x64
                COMMAND lpython --backend wasm_x64 ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py -o ${name}.x64
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py
                VERBATIM
            )
            add_custom_target(${name} ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${name}.x64)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.x64)
            if (RUN_LABELS)
                set_tests_properties(${name} PROPERTIES LABELS "${RUN_LABELS}")
            endif()
            if (${RUN_FAIL})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "wasm")
            # wasm test
            execute_process(COMMAND lpython --backend wasm ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py -o ${name})

            find_program(WASM_EXEC_RUNTIME node)
            execute_process(COMMAND "${WASM_EXEC_RUNTIME}" --version
                            OUTPUT_VARIABLE WASM_EXEC_VERSION
                            OUTPUT_STRIP_TRAILING_WHITESPACE)
            string(COMPARE GREATER_EQUAL "${WASM_EXEC_VERSION}"
                                    "v16.0.0" IS_NODE_ABOVE_16)

            if (NOT IS_NODE_ABOVE_16)
                message(STATUS "${WASM_EXEC_RUNTIME} version: ${WASM_EXEC_VERSION}")
                set(WASM_EXEC_FLAGS "--experimental-wasm-bigint")
            endif()

            add_test(${name} ${WASM_EXEC_RUNTIME} ${WASM_EXEC_FLAGS} ${CMAKE_CURRENT_BINARY_DIR}/${name}.js)
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
RUN(NAME array_01_decl            LABELS cpython llvm c)
RUN(NAME array_02_decl            LABELS cpython llvm c)
RUN(NAME array_03_decl            LABELS cpython llvm c)
RUN(NAME variable_decl_01         LABELS cpython llvm c)
RUN(NAME variable_decl_02         LABELS cpython llvm c)
RUN(NAME array_expr_01            LABELS cpython llvm)
RUN(NAME array_expr_02            LABELS cpython llvm)
RUN(NAME bindc_01            LABELS cpython llvm c)
RUN(NAME bindc_02            LABELS cpython llvm c)
RUN(NAME bindc_04            LABELS llvm c)
RUN(NAME bindc_07            LABELS llvm c)
RUN(NAME exit_01             LABELS cpython llvm c wasm wasm_x86 wasm_x64)
RUN(NAME exit_02     FAIL    LABELS cpython llvm c wasm wasm_x86 wasm_x64)
RUN(NAME exit_01b            LABELS cpython llvm c wasm wasm_x86)
RUN(NAME exit_02b    FAIL    LABELS cpython llvm c wasm wasm_x86)
RUN(NAME exit_02c    FAIL    LABELS cpython llvm c)

# Test all four backends
RUN(NAME print_01            LABELS cpython llvm c wasm) # wasm not yet supports sep and end keywords
RUN(NAME print_03            LABELS x86 c wasm wasm_x86 wasm_x64) # simple test case specifically for x86, wasm_x86 and wasm_x64
RUN(NAME print_04            LABELS cpython llvm c)

# CPython and LLVM
RUN(NAME const_01            LABELS cpython llvm c)
RUN(NAME const_02            LABELS cpython llvm c)
RUN(NAME const_03            LABELS cpython llvm c
        EXTRAFILES const_03b.c)
RUN(NAME const_04            LABELS cpython llvm c)
RUN(NAME expr_01             LABELS cpython llvm c wasm)
RUN(NAME expr_02             LABELS cpython llvm c wasm)
RUN(NAME expr_03             LABELS cpython llvm c wasm)
RUN(NAME expr_04             LABELS cpython llvm c)
RUN(NAME expr_05             LABELS cpython llvm c)
RUN(NAME expr_06             LABELS cpython llvm c)
RUN(NAME expr_07             LABELS cpython llvm c)
RUN(NAME expr_08             LABELS llvm c)
RUN(NAME expr_09             LABELS cpython llvm c)
RUN(NAME expr_10             LABELS cpython llvm c)
RUN(NAME expr_11             LABELS cpython llvm c wasm)
RUN(NAME expr_12             LABELS llvm c)
RUN(NAME expr_13             LABELS llvm c
        EXTRAFILES expr_13b.c)
RUN(NAME expr_14             LABELS cpython llvm c)
RUN(NAME loop_01             LABELS cpython llvm c)
RUN(NAME loop_02             LABELS cpython llvm c wasm wasm_x86)
RUN(NAME if_01               LABELS cpython llvm c wasm wasm_x86)
RUN(NAME if_02               LABELS cpython llvm c wasm wasm_x86)
RUN(NAME print_02            LABELS cpython llvm)
RUN(NAME test_types_01       LABELS cpython llvm c)
RUN(NAME test_str_01         LABELS cpython llvm c)
RUN(NAME test_str_02         LABELS cpython llvm c)
RUN(NAME test_str_03         LABELS cpython llvm c)
RUN(NAME test_list_01        LABELS cpython llvm c)
RUN(NAME test_list_02        LABELS cpython llvm c)
RUN(NAME test_list_03        LABELS cpython llvm c)
RUN(NAME test_list_04        LABELS cpython llvm c)
RUN(NAME test_list_05        LABELS cpython llvm c)
RUN(NAME test_list_06        LABELS cpython llvm c)
RUN(NAME test_list_07        LABELS cpython llvm c)
RUN(NAME test_list_08        LABELS cpython llvm c)
RUN(NAME test_list_09        LABELS cpython llvm c)
RUN(NAME test_list_10        LABELS cpython llvm c)
RUN(NAME test_list_section   LABELS cpython llvm)
RUN(NAME test_tuple_01       LABELS cpython llvm c)
RUN(NAME test_tuple_02       LABELS cpython llvm c)
RUN(NAME test_dict_01        LABELS cpython llvm)
RUN(NAME test_dict_02        LABELS cpython llvm)
RUN(NAME test_dict_03        LABELS cpython llvm)
RUN(NAME test_dict_04        LABELS cpython llvm)
RUN(NAME test_dict_05        LABELS cpython llvm)
RUN(NAME test_for_loop       LABELS cpython llvm c)
RUN(NAME modules_01          LABELS cpython llvm c wasm wasm_x86)
RUN(NAME modules_02          LABELS cpython llvm wasm wasm_x86)
RUN(NAME test_import_01      LABELS cpython llvm c)
RUN(NAME test_import_02      LABELS cpython llvm)
RUN(NAME test_import_03      LABELS cpython llvm)
RUN(NAME test_import_04      IMPORT_PATH ..
        LABELS cpython llvm)
RUN(NAME test_math           LABELS cpython llvm)
RUN(NAME test_numpy_01       LABELS cpython llvm c)
RUN(NAME test_numpy_02       LABELS cpython llvm c)
RUN(NAME test_numpy_03       LABELS cpython llvm c)
RUN(NAME test_numpy_04       LABELS cpython llvm c)
RUN(NAME elemental_01        LABELS cpython llvm)
RUN(NAME elemental_02        LABELS cpython llvm)
RUN(NAME elemental_03        LABELS cpython llvm)
RUN(NAME elemental_04        LABELS cpython llvm)
RUN(NAME elemental_05        LABELS cpython llvm)
RUN(NAME elemental_06        LABELS cpython llvm)
RUN(NAME elemental_07        LABELS cpython llvm)
RUN(NAME elemental_08        LABELS cpython llvm)
RUN(NAME elemental_09        LABELS cpython llvm)
RUN(NAME elemental_10        LABELS cpython llvm)
RUN(NAME elemental_11        LABELS cpython llvm)
RUN(NAME elemental_12        LABELS cpython llvm c)
RUN(NAME test_random         LABELS cpython llvm)
RUN(NAME test_os             LABELS cpython llvm c)
RUN(NAME test_builtin        LABELS cpython llvm c)
RUN(NAME test_builtin_abs    LABELS cpython llvm c)
RUN(NAME test_builtin_bool   LABELS cpython llvm c)
RUN(NAME test_builtin_pow    LABELS cpython llvm c)
RUN(NAME test_builtin_int    LABELS cpython llvm c)
RUN(NAME test_builtin_len    LABELS cpython llvm c)
RUN(NAME test_builtin_str    LABELS cpython llvm c)
RUN(NAME test_builtin_oct    LABELS cpython llvm c)
RUN(NAME test_builtin_hex    LABELS cpython llvm c)
RUN(NAME test_builtin_bin    LABELS cpython llvm c)
RUN(NAME test_builtin_float  LABELS cpython llvm c)
RUN(NAME test_builtin_str_02 LABELS cpython llvm c)
RUN(NAME test_builtin_round  LABELS cpython llvm c)
RUN(NAME test_builtin_divmod LABELS cpython llvm c)
RUN(NAME test_math1          LABELS cpython llvm c)
RUN(NAME test_math_02        LABELS cpython llvm)
RUN(NAME test_pass_compare   LABELS cpython llvm c)
RUN(NAME test_c_interop_01   LABELS cpython llvm c)
RUN(NAME test_c_interop_02   LABELS cpython llvm c
        EXTRAFILES test_c_interop_02b.c)
RUN(NAME test_c_interop_03   LABELS cpython llvm c
        EXTRAFILES test_c_interop_03b.c)
RUN(NAME test_c_interop_04   LABELS cpython llvm c
        EXTRAFILES test_c_interop_04b.c)
RUN(NAME test_c_interop_05   LABELS llvm c
        EXTRAFILES test_c_interop_05b.c)
RUN(NAME bindc_03   LABELS llvm c
        EXTRAFILES bindc_03b.c)
RUN(NAME bindc_05   LABELS llvm c
        EXTRAFILES bindc_05b.c)
RUN(NAME bindc_06   LABELS llvm c
        EXTRAFILES bindc_06b.c)
RUN(NAME test_generics_01    LABELS cpython llvm c)
RUN(NAME test_cmath          LABELS cpython llvm c)
RUN(NAME test_complex        LABELS cpython llvm c)
RUN(NAME test_max_min        LABELS cpython llvm c)
RUN(NAME test_global         LABELS cpython llvm)
RUN(NAME test_integer_bitnot LABELS cpython llvm c)
RUN(NAME test_ifexp          LABELS cpython llvm c)
RUN(NAME test_unary_minus    LABELS cpython llvm c)
RUN(NAME test_unary_plus     LABELS cpython llvm c)
RUN(NAME test_bool_binop     LABELS cpython llvm c)
RUN(NAME test_issue_518      LABELS cpython llvm c)
RUN(NAME structs_01          LABELS cpython llvm c)
RUN(NAME structs_02          LABELS cpython llvm c)
RUN(NAME structs_03          LABELS llvm c)
RUN(NAME structs_04          LABELS cpython llvm c)
RUN(NAME structs_05          LABELS llvm c)
RUN(NAME structs_06          LABELS cpython llvm c)
RUN(NAME structs_07          LABELS llvm c
        EXTRAFILES structs_07b.c)
RUN(NAME structs_08          LABELS cpython llvm c)
RUN(NAME structs_09          LABELS cpython llvm c)
RUN(NAME structs_10          LABELS cpython llvm c)
RUN(NAME structs_11          LABELS cpython llvm c)
RUN(NAME structs_12          LABELS cpython llvm c)
RUN(NAME structs_13          LABELS llvm c
    EXTRAFILES structs_13b.c)
RUN(NAME structs_14          LABELS cpython llvm c)
RUN(NAME structs_15          LABELS cpython llvm c)
RUN(NAME structs_16          LABELS cpython llvm c)
RUN(NAME structs_17          LABELS cpython llvm c)
RUN(NAME structs_18          LABELS llvm c
    EXTRAFILES structs_18b.c)
RUN(NAME sizeof_01           LABELS llvm c
        EXTRAFILES sizeof_01b.c)
RUN(NAME enum_01             LABELS cpython llvm c)
RUN(NAME enum_02             LABELS cpython llvm)
RUN(NAME enum_03             LABELS cpython llvm c)
RUN(NAME enum_04             LABELS cpython llvm c)
RUN(NAME enum_05             LABELS llvm c
        EXTRAFILES enum_05b.c)
RUN(NAME enum_06             LABELS cpython llvm c)
RUN(NAME union_01            LABELS cpython llvm c)
RUN(NAME union_02            LABELS llvm c)
RUN(NAME union_03            LABELS cpython llvm c)
RUN(NAME test_str_to_int     LABELS cpython llvm c)
RUN(NAME test_platform       LABELS cpython llvm c)
RUN(NAME test_vars_01        LABELS cpython llvm)
RUN(NAME test_version        LABELS cpython llvm c)
RUN(NAME vec_01              LABELS cpython llvm c)
RUN(NAME test_str_comparison LABELS cpython llvm c)
RUN(NAME test_bit_length     LABELS cpython llvm c)
RUN(NAME str_to_list_cast    LABELS cpython llvm c)

RUN(NAME generics_01         LABELS cpython llvm)
RUN(NAME generics_02         LABELS cpython llvm)
RUN(NAME generics_array_01   LABELS cpython llvm)
RUN(NAME generics_array_02   LABELS cpython llvm)
RUN(NAME generics_array_03   LABELS cpython llvm)
RUN(NAME generics_list_01    LABELS cpython llvm)
RUN(NAME test_statistics     LABELS cpython llvm)
RUN(NAME test_str_attributes LABELS cpython llvm c)
RUN(NAME kwargs_01           LABELS cpython llvm c)
RUN(NAME test_01_goto        LABELS cpython llvm c)

RUN(NAME func_inline_01 LABELS llvm c wasm)
RUN(NAME func_inline_02 LABELS cpython llvm c)
RUN(NAME func_static_01 LABELS cpython llvm c wasm)
RUN(NAME func_static_02 LABELS cpython llvm c wasm)
RUN(NAME func_dep_03    LABELS cpython llvm c)
RUN(NAME func_dep_04    LABELS cpython llvm c)
