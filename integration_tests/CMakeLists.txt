cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

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
set(FAST no CACHE BOOL "Run supported tests with --fast")

find_program(LPYTHON NAMES lpython)

execute_process(COMMAND ${LPYTHON} --get-rtl-header-dir
    OUTPUT_VARIABLE LPYTHON_RTL_HEADER_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
execute_process(COMMAND ${LPYTHON} --get-rtl-dir
    OUTPUT_VARIABLE LPYTHON_RTL_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

find_path(LPYTHON_RTLIB_DIR lfortran_intrinsics.h
    HINTS ${LPYTHON_RTL_HEADER_DIR})
find_library(LPYTHON_RTLIB_LIBRARY lpython_runtime_static
    HINTS ${LPYTHON_RTL_DIR})
add_library(lpython_rtlib INTERFACE IMPORTED)
set_property(TARGET lpython_rtlib PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${LPYTHON_RTLIB_DIR})
set_property(TARGET lpython_rtlib PROPERTY INTERFACE_LINK_LIBRARIES
    ${LPYTHON_RTLIB_LIBRARY})
target_link_libraries(lpython_rtlib INTERFACE m)

if (PYTHON_LIBS_REQ)
    find_package(Python COMPONENTS Interpreter Development)
    execute_process(
        COMMAND "${Python_EXECUTABLE}"
        -c "import numpy; print(numpy.get_include())"
        OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message("\n")
    message("System has the Python development artifacts: ${Python_Development_FOUND}")
    message("The Python include directories: ${Python_INCLUDE_DIRS}")
    message("The Python libraries: ${Python_LIBRARIES}")
    message("The Python library directories: ${Python_LIBRARY_DIRS}")
    message("The Python runtime library directories: ${Python_RUNTIME_LIBRARY_DIRS}")
    message("Python version: ${Python_VERSION}")
    message("Numpy Include Directory: ${NUMPY_INCLUDE_DIR}")
endif()

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
message("FAST: ${FAST}")
message("PYTHON_LIBS_REQ: ${PYTHON_LIBS_REQ}")
message("LPYTHON: ${LPYTHON}")
message("LPYTHON_RTLIB_DIR: ${LPYTHON_RTLIB_DIR}")
message("LPYTHON_RTLIB_LIBRARY: ${LPYTHON_RTLIB_LIBRARY}")


macro(RUN_UTIL RUN_FAIL RUN_NAME RUN_FILE_NAME RUN_LABELS RUN_EXTRAFILES RUN_NOMOD RUN_EXTRA_ARGS RUN_COPY_TO_BIN)
    set(fail ${${RUN_FAIL}})
    set(name ${${RUN_NAME}})
    set(file_name ${${RUN_FILE_NAME}})
    set(labels ${${RUN_LABELS}})
    set(extra_files ${${RUN_EXTRAFILES}})
    set(no_mod ${${RUN_NOMOD}})
    set(extra_args ${${RUN_EXTRA_ARGS}})
    set(copy_to_bin ${${RUN_COPY_TO_BIN}})

    if (NOT name)
        message(FATAL_ERROR "Must specify the NAME argument")
    endif()

    if (${KIND} IN_LIST labels)
        if (KIND STREQUAL "llvm")
            add_custom_command(
                OUTPUT ${name}.o
                COMMAND ${LPYTHON} -c ${extra_args} ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py -o ${name}.o
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py
                VERBATIM)
            add_executable(${name} ${name}.o ${extra_files})
            set_target_properties(${name} PROPERTIES LINKER_LANGUAGE C)
            target_link_libraries(${name} lpython_rtlib)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif (KIND STREQUAL "llvm_jit")
            add_test(
                NAME ${name}
                COMMAND ${LPYTHON} --jit ${extra_args} ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py)
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif (KIND STREQUAL "llvm_py")
            add_custom_command(
                OUTPUT ${name}.o
                COMMAND ${LPYTHON} -c ${extra_args} ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py -o ${name}.o
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py
                VERBATIM)
            add_executable(${name} ${name}.o ${extra_files})
            target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR} ${NUMPY_INCLUDE_DIR})
            set_target_properties(${name} PROPERTIES LINKER_LANGUAGE C)
            target_link_libraries(${name} lpython_rtlib Python::Python)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "llvm_sym")
            add_custom_command(
                OUTPUT ${name}.o
                COMMAND ${LPYTHON} -c ${extra_args} ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py -o ${name}.o
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py
                VERBATIM)
            add_executable(${name} ${name}.o ${extra_files})
            set_target_properties(${name} PROPERTIES LINKER_LANGUAGE C)
            if (APPLE)
                set(SYMENGINE_LIB "${Python_LIBRARY_DIRS}/libsymengine.dylib")
            else()
                set(SYMENGINE_LIB "${Python_LIBRARY_DIRS}/libsymengine.so")
            endif()
            target_link_libraries(${name} lpython_rtlib ${SYMENGINE_LIB})
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "c")
            add_custom_command(
                OUTPUT ${name}.c
                COMMAND ${LPYTHON} ${extra_args} --show-c ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py > ${name}.c
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py
                VERBATIM)
            add_executable(${name} ${name}.c ${extra_files})
            target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR})
            set_target_properties(${name} PROPERTIES LINKER_LANGUAGE C)
            target_link_libraries(${name} lpython_rtlib)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "c_py")
            add_custom_command(
                OUTPUT ${name}.c
                COMMAND ${LPYTHON} ${extra_args} --show-c ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py > ${name}.c
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py
                VERBATIM)
            add_executable(${name} ${name}.c ${extra_files})
            target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR} ${NUMPY_INCLUDE_DIR})
            set_target_properties(${name} PROPERTIES LINKER_LANGUAGE C)
            target_link_libraries(${name} lpython_rtlib Python::Python)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "c_sym")
            add_custom_command(
                OUTPUT ${name}.c
                COMMAND ${LPYTHON} ${extra_args} --show-c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py > ${name}.c
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py
                VERBATIM)
            add_executable(${name} ${name}.c ${extra_files})
            target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}
                "${Python_INCLUDE_DIRS}/..")
            set_target_properties(${name} PROPERTIES LINKER_LANGUAGE C)
            if (APPLE)
                set(SYMENGINE_LIB "${Python_LIBRARY_DIRS}/libsymengine.dylib")
            else()
                set(SYMENGINE_LIB "${Python_LIBRARY_DIRS}/libsymengine.so")
            endif()
            target_link_libraries(${name} lpython_rtlib ${SYMENGINE_LIB})
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif((KIND STREQUAL "cpython") OR (KIND STREQUAL "cpython_sym"))
            # CPython test
            if (extra_files)
                set(PY_MOD "${name}_mod")
                add_library(${PY_MOD} SHARED ${extra_files})
                set_target_properties(${PY_MOD} PROPERTIES LINKER_LANGUAGE C)
            else()
                set(PY_MOD "")
            endif()

            add_test(${name} python ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py)
            if (no_mod)
                set_tests_properties(${name} PROPERTIES
                    ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}/../src/runtime/lpython:${CMAKE_SOURCE_DIR}/..")
            else()
                set_tests_properties(${name} PROPERTIES
                    ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}/../src/runtime/lpython:${CMAKE_SOURCE_DIR}/..;LPYTHON_PY_MOD_NAME=${PY_MOD};LPYTHON_PY_MOD_PATH=${CMAKE_CURRENT_BINARY_DIR}")
            endif()
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "x86")
            # x86 test
            add_custom_command(
                OUTPUT ${name}.x86
                COMMAND ${LPYTHON} ${extra_args} --backend x86 ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py -o ${name}.x86
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py
                VERBATIM
            )
            add_custom_target(${name} ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${name}.x86)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.x86)
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "wasm_x86")
            # wasm_to_x86 test
            add_custom_command(
                OUTPUT ${name}.x86
                COMMAND ${LPYTHON} ${extra_args} --backend wasm_x86 ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py -o ${name}.x86
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py
                VERBATIM
            )
            add_custom_target(${name} ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${name}.x86)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.x86)
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "wasm_x64")
            # wasm_to_x64 test
            add_custom_command(
                OUTPUT ${name}.x64
                COMMAND ${LPYTHON} ${extra_args} --backend wasm_x64 ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py -o ${name}.x64
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py
                VERBATIM
            )
            add_custom_target(${name} ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${name}.x64)
            add_test(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.x64)
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        elseif(KIND STREQUAL "wasm")
            # wasm test
            execute_process(COMMAND ${LPYTHON} ${extra_args} --backend wasm ${CMAKE_CURRENT_SOURCE_DIR}/${file_name}.py -o ${name})

            find_program(WASM_EXEC_RUNTIME node)
            execute_process(COMMAND "${WASM_EXEC_RUNTIME}" --version
                            OUTPUT_VARIABLE WASM_EXEC_VERSION
                            OUTPUT_STRIP_TRAILING_WHITESPACE)

            string(REGEX REPLACE "v([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" NODE_MAJOR_VERSION "${WASM_EXEC_VERSION}")

            if (NODE_MAJOR_VERSION LESS 16)
                message(STATUS "${WASM_EXEC_RUNTIME} version: ${WASM_EXEC_VERSION}")
                set(WASM_EXEC_FLAGS "--experimental-wasm-bigint")
            endif()
            set(WASM_EXEC_FLAGS ${WASM_EXEC_FLAGS} "--experimental-wasi-unstable-preview1")
            add_test(${name} ${WASM_EXEC_RUNTIME} ${WASM_EXEC_FLAGS} ${CMAKE_CURRENT_BINARY_DIR}/${name}.js)
            if (labels)
                set_tests_properties(${name} PROPERTIES LABELS "${labels}")
            endif()
            if (${fail})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        endif()

        if (copy_to_bin)
            file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${copy_to_bin} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
        endif()

    endif()
endmacro(RUN_UTIL)

macro(RUN)
    set(options FAIL NOFAST NOMOD)
    set(oneValueArgs NAME IMPORT_PATH COPY_TO_BIN REQ_PY_VER)
    set(multiValueArgs LABELS EXTRAFILES EXTRA_ARGS)
    cmake_parse_arguments(RUN "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN} )

    set(RUN_FILE_NAME ${RUN_NAME})

    if (RUN_IMPORT_PATH)
        # Only one import path supported for now
        # Later add support for multiple import paths by looping over and appending to extra args
        set(RUN_EXTRA_ARGS ${RUN_EXTRA_ARGS} -I${CMAKE_CURRENT_SOURCE_DIR}/${RUN_IMPORT_PATH})
    endif()

    if (RUN_REQ_PY_VER)
        string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" PY_MAJOR_VERSION "${Python_VERSION}")
        string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" PY_MINOR_VERSION "${Python_VERSION}")
        string(REGEX REPLACE "([0-9]+)\\.([0-9]+)" "\\1" REQ_PY_MAJOR_VERSION "${RUN_REQ_PY_VER}")
        string(REGEX REPLACE "([0-9]+)\\.([0-9]+)" "\\2" REQ_PY_MINOR_VERSION "${RUN_REQ_PY_VER}")

        if (PY_MINOR_VERSION LESS REQ_PY_MINOR_VERSION)
            # remove backends from the test that depend on CPython
            list(REMOVE_ITEM RUN_LABELS cpython cpython_sym c_py c_sym llvm_sym llvm_py)
        endif()
    endif()

    if (NOT FAST)
        RUN_UTIL(RUN_FAIL RUN_NAME RUN_FILE_NAME RUN_LABELS RUN_EXTRAFILES RUN_NOMOD RUN_EXTRA_ARGS RUN_COPY_TO_BIN)
    endif()

    if ((FAST) AND (NOT RUN_NOFAST))
        set(RUN_EXTRA_ARGS ${RUN_EXTRA_ARGS} --fast)
        set(RUN_NAME "${RUN_NAME}_FAST")
        list(REMOVE_ITEM RUN_LABELS cpython cpython_sym) # remove cpython, cpython_sym, from --fast test
        RUN_UTIL(RUN_FAIL RUN_NAME RUN_FILE_NAME RUN_LABELS RUN_EXTRAFILES RUN_NOMOD RUN_EXTRA_ARGS RUN_COPY_TO_BIN)
    endif()
endmacro(RUN)

# only compiles till object file
# to make sure that the generated code is syntactically correct
# but we cannot generate an executable due to --disable-main option enabled.
macro(COMPILE)
    set(options FAIL)
    set(oneValueArgs NAME IMPORT_PATH)
    set(multiValueArgs LABELS EXTRAFILES)
    cmake_parse_arguments(COMPILE "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )
    set(name ${COMPILE_NAME})
    if (NOT name)
        message(FATAL_ERROR "Must specify the NAME argument")
    endif()

    set(extra_args "")
    if (COMPILE_IMPORT_PATH)
        # Only one import path supported for now
        # Later add support for multiple import paths by looping over and appending to extra args
        set(extra_args ${extra_args} -I${CMAKE_CURRENT_SOURCE_DIR}/${COMPILE_IMPORT_PATH})
    endif()

    if (${KIND} IN_LIST COMPILE_LABELS)
        if (KIND STREQUAL "llvm")
            add_custom_command(
                OUTPUT ${name}.o
                COMMAND ${LPYTHON} ${extra_args} --disable-main -c ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py -o ${name}.o
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py
                VERBATIM)
            add_library(${name} OBJECT ${name}.o)
            set_target_properties(${name} PROPERTIES LINKER_LANGUAGE C)
        elseif(KIND STREQUAL "c")
            add_custom_command(
                OUTPUT ${name}.c
                COMMAND ${LPYTHON} ${extra_args} --disable-main --show-c ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py > ${name}.c
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py
                VERBATIM)
            add_library(${name} OBJECT ${name}.c)
            set_target_properties(${name} PROPERTIES LINKER_LANGUAGE C)
            target_link_libraries(${name} lpython_rtlib)
        elseif(KIND STREQUAL "cpython")
            # CPython test
            set(PY_MOD "")

            add_test(${name} python ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py)
            set_tests_properties(${name} PROPERTIES
                ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}/../src/runtime/lpython:${CMAKE_SOURCE_DIR}/..;LPYTHON_PY_MOD_NAME=${PY_MOD};LPYTHON_PY_MOD_PATH=${CMAKE_CURRENT_BINARY_DIR}")
            if (RUN_LABELS)
                set_tests_properties(${name} PROPERTIES LABELS "${RUN_LABELS}")
            endif()
            if (${RUN_FAIL})
                set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
            endif()
        endif()
    endif()
endmacro(COMPILE)


# Test zero and non-zero exit code and assert statements
RUN(NAME array_01_decl            LABELS cpython llvm llvm_jit c)
RUN(NAME array_02_decl            LABELS cpython llvm llvm_jit c)
RUN(NAME array_03_decl            LABELS cpython llvm llvm_jit c)
RUN(NAME variable_decl_01         LABELS cpython llvm llvm_jit c)
RUN(NAME variable_decl_02         LABELS cpython llvm llvm_jit c)
RUN(NAME variable_decl_03         LABELS cpython llvm llvm_jit c)
RUN(NAME array_expr_01            LABELS cpython llvm llvm_jit c)
RUN(NAME array_expr_02            LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME array_expr_03            LABELS cpython llvm llvm_jit c)
RUN(NAME array_expr_04            LABELS cpython llvm llvm_jit c)
# RUN(NAME array_expr_05            LABELS cpython llvm llvm_jit c)
RUN(NAME array_expr_06            LABELS cpython llvm llvm_jit c)
# RUN(NAME array_expr_07            LABELS cpython llvm llvm_jit c)
RUN(NAME array_expr_08            LABELS cpython llvm llvm_jit c)
RUN(NAME array_expr_09            LABELS cpython llvm llvm_jit c)
RUN(NAME array_expr_10            LABELS cpython llvm llvm_jit c)
RUN(NAME array_size_01            LABELS cpython llvm llvm_jit c)
RUN(NAME array_size_02            LABELS cpython llvm llvm_jit c)
RUN(NAME array_01            LABELS cpython llvm llvm_jit wasm c)
RUN(NAME array_02            LABELS cpython wasm c)
RUN(NAME array_03            LABELS cpython llvm llvm_jit c)
RUN(NAME array_04            LABELS cpython llvm llvm_jit c)
RUN(NAME array_05            LABELS cpython llvm llvm_jit c)
RUN(NAME array_06            LABELS cpython llvm llvm_jit)
# RUN(NAME bindc_01            LABELS cpython llvm llvm_jit c)
# # RUN(NAME bindc_02            LABELS cpython llvm llvm_jit c)
# RUN(NAME bindc_04            LABELS llvm llvm_jit c NOFAST)
# # RUN(NAME bindc_07            LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME bindc_08            LABELS cpython llvm llvm_jit c)
# RUN(NAME bindc_09            LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME bindc_09b           LABELS cpython llvm llvm_jit c NOFAST)
# # RUN(NAME bindc_10            LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME bindc_11            LABELS cpython) # This is CPython test only
# RUN(NAME exit_01             LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME exit_02     FAIL    LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME exit_03             LABELS cpython llvm llvm_jit c wasm wasm_x86 wasm_x64)
RUN(NAME exit_04     FAIL    LABELS cpython llvm llvm_jit c wasm wasm_x86 wasm_x64)
RUN(NAME exit_01b            LABELS cpython llvm llvm_jit c wasm wasm_x86 wasm_x64)
RUN(NAME exit_02b    FAIL    LABELS cpython llvm llvm_jit c wasm wasm_x86 wasm_x64)
RUN(NAME exit_02c    FAIL    LABELS cpython llvm llvm_jit c)

# Test all four backends
RUN(NAME print_01            LABELS cpython llvm llvm_jit wasm) # renable c, wasm not yet supports sep and end keywords
RUN(NAME print_03            LABELS x86 c wasm wasm_x86 wasm_x64) # simple test case specifically for x86, wasm_x86 and wasm_x64
RUN(NAME print_04            LABELS cpython llvm llvm_jit c)
RUN(NAME print_06            LABELS cpython llvm llvm_jit) # renable c 
RUN(NAME print_05            LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME print_float         LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME print_list_tuple_01    LABELS cpython llvm llvm_jit NOFAST) # renable c
# # RUN(NAME print_list_tuple_02    LABELS cpython llvm llvm_jit c NOFAST)
# # RUN(NAME print_list_tuple_03    LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME test_list_item_mixed_print    LABELS cpython llvm llvm_jit NOFAST) # renable c
RUN(NAME test_intrinsic_function_mixed_print    LABELS cpython llvm llvm_jit NOFAST)

# # CPython and LLVM
RUN(NAME const_01            LABELS cpython llvm llvm_jit c wasm)
RUN(NAME const_02            LABELS cpython llvm llvm_jit c wasm)
RUN(NAME const_03            LABELS cpython llvm c
        EXTRAFILES const_03b.c)
# RUN(NAME const_04            LABELS cpython llvm llvm_jit c)
RUN(NAME expr_01             LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME expr_02             LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME expr_03             LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME expr_04             LABELS cpython llvm llvm_jit c wasm NOFAST)
RUN(NAME expr_05             LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME expr_06             LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME expr_07             LABELS cpython llvm llvm_jit c)
# RUN(NAME expr_08             LABELS llvm llvm_jit c NOFAST)
RUN(NAME expr_09             LABELS cpython llvm llvm_jit c)
RUN(NAME expr_10             LABELS cpython llvm llvm_jit c)
RUN(NAME expr_11             LABELS cpython llvm llvm_jit c wasm)
# RUN(NAME expr_12             LABELS llvm llvm_jit c)
# # RUN(NAME expr_13             LABELS llvm c
# #         EXTRAFILES expr_13b.c NOFAST)
RUN(NAME expr_14             LABELS cpython llvm llvm_jit c)
RUN(NAME expr_15             LABELS cpython llvm llvm_jit c)
# # RUN(NAME expr_16             LABELS cpython llvm llvm_jit c)
RUN(NAME expr_17             LABELS cpython llvm llvm_jit c)
RUN(NAME expr_18    FAIL     LABELS cpython llvm llvm_jit c)
RUN(NAME expr_19             LABELS cpython llvm llvm_jit c)
RUN(NAME expr_20             LABELS cpython llvm llvm_jit c)
RUN(NAME expr_21             LABELS cpython llvm llvm_jit c)
RUN(NAME expr_22             LABELS cpython llvm llvm_jit c)
RUN(NAME expr_23             LABELS cpython llvm llvm_jit c)
# RUN(NAME expr_24             LABELS cpython wasm) # mandelbrot

RUN(NAME expr_01u            LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME expr_02u            LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME expr_03u            LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME expr_04u            LABELS cpython llvm llvm_jit c)

RUN(NAME list_01             LABELS cpython llvm llvm_jit)

RUN(NAME loop_01             LABELS cpython llvm llvm_jit) # renable c 
RUN(NAME loop_02             LABELS cpython llvm llvm_jit c wasm wasm_x86 wasm_x64)
RUN(NAME loop_03             LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME loop_04             LABELS cpython llvm llvm_jit c)
RUN(NAME loop_05             LABELS cpython llvm llvm_jit c)
# RUN(NAME loop_06             LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME loop_07             LABELS cpython llvm llvm_jit c)
RUN(NAME loop_08             LABELS cpython llvm llvm_jit c)
RUN(NAME loop_09             LABELS cpython llvm llvm_jit)
RUN(NAME loop_10             LABELS cpython llvm llvm_jit)
# RUN(NAME loop_11             LABELS cpython llvm llvm_jit)
RUN(NAME if_01               LABELS cpython llvm llvm_jit c wasm wasm_x86 wasm_x64)
RUN(NAME if_02               LABELS cpython llvm llvm_jit c wasm wasm_x86 wasm_x64)
# RUN(NAME if_03    FAIL       LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME print_02            LABELS cpython llvm llvm_jit) # renable c
RUN(NAME test_types_01       LABELS cpython llvm llvm_jit c)
RUN(NAME test_types_02       LABELS cpython llvm llvm_jit c wasm)
# RUN(NAME test_str_01         LABELS cpython llvm llvm_jit c)
# RUN(NAME test_str_02         LABELS cpython llvm llvm_jit) # renable c
# RUN(NAME test_str_03         LABELS cpython llvm llvm_jit) # renable c
# RUN(NAME test_str_04         LABELS cpython llvm llvm_jit wasm) # renable c
# RUN(NAME test_str_05         LABELS cpython llvm llvm_jit c)
# RUN(NAME test_str_06         LABELS cpython llvm llvm_jit c)
RUN(NAME test_string_01      LABELS cpython llvm llvm_jit) # renable c
# RUN(NAME test_list_01        LABELS cpython llvm llvm_jit c)
# RUN(NAME test_list_02        LABELS cpython llvm llvm_jit c)
# RUN(NAME test_list_03        LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME test_list_04        LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME test_list_05        LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME test_list_06        LABELS cpython llvm llvm_jit c)
# RUN(NAME test_list_07        LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME test_list_08        LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME test_list_09        LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME test_list_10        LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME test_list_11        LABELS cpython llvm llvm_jit c)
# RUN(NAME test_list_section   LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME test_list_section2  LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME test_list_count     LABELS cpython llvm llvm_jit) 
RUN(NAME test_list_index     LABELS cpython llvm llvm_jit)
RUN(NAME test_list_index2    LABELS cpython llvm llvm_jit)
# RUN(NAME test_list_repeat    LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME test_list_repeat2   LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME test_list_reverse   LABELS cpython llvm llvm_jit) 
# RUN(NAME test_list_pop       LABELS cpython llvm llvm_jit NOFAST) # TODO: Remove NOFAST from here.
# RUN(NAME test_list_pop2       LABELS cpython llvm llvm_jit NOFAST) # TODO: Remove NOFAST from here.
RUN(NAME test_list_pop3      LABELS cpython llvm llvm_jit)
RUN(NAME test_list_compare   LABELS cpython llvm llvm_jit) 
RUN(NAME test_list_compare2   LABELS cpython llvm llvm_jit)
# RUN(NAME test_list_concat    LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME test_list_reserve   LABELS cpython llvm llvm_jit)
RUN(NAME test_const_list     LABELS cpython llvm llvm_jit)
RUN(NAME test_const_access     LABELS cpython llvm llvm_jit)
RUN(NAME test_tuple_01       LABELS cpython llvm llvm_jit) # renable c
# RUN(NAME test_tuple_02       LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME test_tuple_03       LABELS cpython llvm llvm_jit) # renable c
RUN(NAME test_tuple_04       LABELS cpython llvm llvm_jit) # renable c
RUN(NAME test_tuple_concat   LABELS cpython llvm llvm_jit) 
RUN(NAME test_tuple_nested   LABELS cpython llvm llvm_jit)
RUN(NAME test_const_dict     LABELS cpython llvm llvm_jit) 
# RUN(NAME test_params         LABELS cpython llvm llvm_jit NOFAST)
RUN(NAME test_dict_01        LABELS cpython llvm llvm_jit c) 
RUN(NAME test_dict_02        LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME test_dict_03        LABELS cpython llvm llvm_jit NOFAST) 
# RUN(NAME test_dict_04        LABELS cpython llvm llvm_jit NOFAST)
RUN(NAME test_dict_05        LABELS cpython llvm llvm_jit c)
# RUN(NAME test_dict_06        LABELS cpython llvm llvm_jit c)
# RUN(NAME test_dict_07        LABELS cpython llvm llvm_jit c)
# RUN(NAME test_dict_08        LABELS cpython llvm llvm_jit c)
# RUN(NAME test_dict_09        LABELS cpython llvm llvm_jit c)
RUN(NAME test_dict_10        LABELS cpython llvm llvm_jit) # renable c 
RUN(NAME test_dict_11        LABELS cpython llvm llvm_jit c) 
RUN(NAME test_dict_12        LABELS cpython llvm llvm_jit) # renable c
# RUN(NAME test_dict_13        LABELS cpython llvm llvm_jit c)
RUN(NAME test_dict_bool      LABELS cpython llvm llvm_jit)
# RUN(NAME test_dict_increment LABELS cpython llvm llvm_jit) 
# RUN(NAME test_dict_keys_values LABELS cpython llvm llvm_jit)
# RUN(NAME test_dict_nested1   LABELS cpython llvm llvm_jit)
# RUN(NAME test_dict_clear     LABELS cpython llvm)
RUN(NAME test_set_len        LABELS cpython llvm llvm_jit) 
RUN(NAME test_set_add        LABELS cpython llvm llvm_jit) 
# RUN(NAME test_set_remove     LABELS cpython llvm llvm_jit)
# RUN(NAME test_set_discard     LABELS cpython llvm llvm_jit)
# RUN(NAME test_set_from_list     LABELS cpython llvm llvm_jit)
# RUN(NAME test_set_clear     LABELS cpython llvm)
# RUN(NAME test_set_pop     LABELS cpython llvm)
RUN(NAME test_global_set     LABELS cpython llvm llvm_jit) 
RUN(NAME test_for_loop       LABELS cpython llvm llvm_jit c)
RUN(NAME modules_01          LABELS cpython llvm llvm_jit c wasm wasm_x86 wasm_x64)
RUN(NAME modules_02          LABELS cpython llvm llvm_jit c wasm wasm_x86 wasm_x64)
RUN(NAME test_import_01      LABELS cpython llvm llvm_jit) # renable c 
RUN(NAME test_import_02      LABELS cpython llvm llvm_jit c)
RUN(NAME test_import_03      LABELS cpython llvm llvm_jit) # renable c 
RUN(NAME test_import_04      LABELS cpython llvm llvm_jit c)
RUN(NAME test_import_05      LABELS cpython llvm llvm_jit c wasm wasm_x86 wasm_x64)
RUN(NAME test_import_06      LABELS cpython llvm llvm_jit)
RUN(NAME test_import_07      LABELS cpython llvm llvm_jit c)
RUN(NAME test_import_08      LABELS cpython llvm)
# RUN(NAME test_math           LABELS cpython llvm llvm_jit NOFAST)
# RUN(NAME test_membership_01  LABELS cpython llvm)
RUN(NAME test_numpy_01       LABELS cpython llvm llvm_jit c)
RUN(NAME test_numpy_02       LABELS cpython llvm llvm_jit c)
RUN(NAME test_numpy_03       LABELS cpython llvm llvm_jit c)
RUN(NAME test_numpy_04       LABELS cpython llvm llvm_jit c)
RUN(NAME elemental_01        LABELS cpython llvm llvm_jit NOFAST) # renable c
# RUN(NAME elemental_02        LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME elemental_03        LABELS cpython llvm llvm_jit NOFAST) # renable c
# RUN(NAME elemental_04        LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME elemental_05        LABELS cpython llvm llvm_jit NOFAST) # renable c
# RUN(NAME elemental_06        LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME elemental_07        LABELS cpython llvm llvm_jit NOFAST) # renable c
RUN(NAME elemental_08        LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME elemental_09        LABELS cpython llvm llvm_jit NOFAST) # renable c
RUN(NAME elemental_10        LABELS cpython llvm llvm_jit NOFAST) # renable c
RUN(NAME elemental_11        LABELS cpython llvm llvm_jit NOFAST) # renable c
RUN(NAME elemental_12        LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME elemental_13        LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME test_random         LABELS cpython llvm llvm_jit NOFAST)
RUN(NAME test_random_02         LABELS cpython llvm llvm_jit NOFAST)
RUN(NAME test_os             LABELS cpython llvm llvm_jit NOFAST) # renable c  
RUN(NAME test_builtin        LABELS cpython llvm llvm_jit) # renable c  
RUN(NAME test_builtin_abs    LABELS cpython llvm llvm_jit c)
# RUN(NAME test_builtin_bool   LABELS cpython llvm llvm_jit c)
# RUN(NAME test_builtin_pow    LABELS cpython llvm llvm_jit c EXTRA_ARGS --no-warnings)
# RUN(NAME test_builtin_int    LABELS cpython llvm llvm_jit c)
# RUN(NAME test_builtin_len    LABELS cpython llvm llvm_jit c)
# RUN(NAME test_builtin_str    LABELS cpython llvm llvm_jit c)
# RUN(NAME test_builtin_oct    LABELS cpython llvm llvm_jit c)
# RUN(NAME test_builtin_hex    LABELS cpython llvm llvm_jit c)
# RUN(NAME test_builtin_bin    LABELS cpython llvm llvm_jit c)
RUN(NAME test_builtin_float  LABELS cpython llvm llvm_jit c)
# RUN(NAME test_builtin_str_02 LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME test_builtin_round  LABELS cpython llvm llvm_jit c)
# RUN(NAME test_builtin_divmod LABELS cpython llvm llvm_jit c)
# RUN(NAME test_builtin_sum    LABELS cpython llvm llvm_jit c)
# RUN(NAME test_math1          LABELS cpython llvm llvm_jit c)
# RUN(NAME test_math_02        LABELS cpython llvm llvm_jit NOFAST)
# RUN(NAME test_math_03        LABELS llvm llvm_jit)               #1595: TODO: Test using CPython (3.11 recommended)
RUN(NAME test_pass_compare   LABELS cpython llvm llvm_jit) # renable c
RUN(NAME test_c_interop_01   LABELS cpython llvm llvm_jit c)
# RUN(NAME test_c_interop_02   LABELS cpython llvm c
#         EXTRAFILES test_c_interop_02b.c)
RUN(NAME test_c_interop_03   LABELS cpython llvm c
        EXTRAFILES test_c_interop_03b.c)
# RUN(NAME test_c_interop_04   LABELS cpython llvm llvm_jit c
#         EXTRAFILES test_c_interop_04b.c)
# RUN(NAME test_c_interop_05   LABELS llvm c
#         EXTRAFILES test_c_interop_05b.c)
# RUN(NAME bindc_03   LABELS llvm c
#         EXTRAFILES bindc_03b.c)
# RUN(NAME bindc_05   LABELS llvm c
#         EXTRAFILES bindc_05b.c)
# RUN(NAME bindc_06   LABELS llvm
#         EXTRAFILES bindc_06b.c) # renable c
# RUN(NAME bindpy_01           LABELS cpython llvm_py c_py EXTRA_ARGS --enable-cpython NOFAST COPY_TO_BIN bindpy_01_module.py)
# RUN(NAME bindpy_02           LABELS cpython c_py EXTRA_ARGS --link-numpy COPY_TO_BIN bindpy_02_module.py)
# RUN(NAME bindpy_03           LABELS cpython c_py EXTRA_ARGS --link-numpy NOFAST COPY_TO_BIN bindpy_03_module.py)
# RUN(NAME bindpy_04           LABELS cpython c_py EXTRA_ARGS --link-numpy NOFAST COPY_TO_BIN bindpy_04_module.py)
# RUN(NAME bindpy_05           LABELS llvm_py c_py EXTRA_ARGS --enable-cpython COPY_TO_BIN bindpy_05_module.py REQ_PY_VER 3.10)
# RUN(NAME bindpy_06           LABELS cpython llvm_py EXTRA_ARGS --enable-cpython NOFAST COPY_TO_BIN bindpy_06_module.py REQ_PY_VER 3.10)
RUN(NAME test_generics_01    LABELS cpython llvm llvm_jit NOFAST) # renable c 
RUN(NAME test_cmath          LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME test_complex_01        LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME test_complex_02        LABELS cpython llvm llvm_jit c)
RUN(NAME test_ConstantEllipsis  LABLES cpython llvm llvm_jit c)
RUN(NAME test_max_min        LABELS cpython llvm llvm_jit c)
RUN(NAME test_global         LABELS cpython llvm llvm_jit) # renable c
RUN(NAME test_global_decl    LABELS cpython llvm llvm_jit) # renable c
RUN(NAME test_ifexp_01          LABELS cpython llvm llvm_jit c)
RUN(NAME test_ifexp_02          LABELS cpython llvm llvm_jit c)
RUN(NAME test_ifexp_03          LABELS cpython llvm llvm_jit c)
RUN(NAME test_unary_op_01    LABELS cpython llvm llvm_jit c) # unary minus
RUN(NAME test_unary_op_02    LABELS cpython llvm llvm_jit c) # unary plus
RUN(NAME test_unary_op_03    LABELS cpython llvm llvm_jit c wasm) # unary bitinvert
RUN(NAME test_unary_op_04    LABELS cpython llvm llvm_jit c) # unary bitinvert
# RUN(NAME test_unary_op_05    LABELS cpython llvm llvm_jit c) # unsigned unary minus, plus
# RUN(NAME test_unary_op_06    LABELS cpython llvm llvm_jit c) # unsigned unary bitnot
# RUN(NAME test_unsigned_01    LABELS cpython llvm llvm_jit c) # unsigned bitshift left, right
# RUN(NAME test_unsigned_02    LABELS cpython llvm llvm_jit c)
# RUN(NAME test_unsigned_03    LABELS cpython llvm llvm_jit c)
RUN(NAME test_bool_binop     LABELS cpython llvm llvm_jit c)
RUN(NAME test_issue_518      LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME structs_01          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_02          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_02b         LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME structs_03          LABELS llvm llvm_jit c)
# RUN(NAME structs_04          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_05          LABELS cpython llvm llvm_jit c)
RUN(NAME structs_06          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_07          LABELS llvm c
#         EXTRAFILES structs_07b.c)
# RUN(NAME structs_08          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_09          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_10          LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME structs_11          LABELS cpython llvm llvm_jit c)
RUN(NAME structs_12          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_13          LABELS llvm c
#     EXTRAFILES structs_13b.c)
RUN(NAME structs_14          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_15          LABELS cpython llvm llvm_jit c)
RUN(NAME structs_16          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_17          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_18          LABELS cpython llvm c
#     EXTRAFILES structs_18b.c)
# RUN(NAME structs_19          LABELS cpython llvm c
#     EXTRAFILES structs_19b.c)
# RUN(NAME structs_20          LABELS cpython llvm c
#     EXTRAFILES structs_20b.c)
RUN(NAME structs_21          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_22          LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME structs_23          LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME structs_24          LABELS cpython llvm llvm_jit c)
RUN(NAME structs_25          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_26          LABELS cpython llvm llvm_jit) # renable c
# RUN(NAME structs_27          LABELS cpython llvm llvm_jit) # renable c
# RUN(NAME structs_28          LABELS cpython llvm llvm_jit) # renable c
RUN(NAME structs_29          LABELS cpython llvm llvm_jit)
# RUN(NAME structs_30          LABELS cpython llvm llvm_jit) # renable c
# RUN(NAME structs_31          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_32          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_33          LABELS cpython llvm llvm_jit c)
# RUN(NAME structs_34          LABELS cpython llvm llvm_jit c)
RUN(NAME structs_35          LABELS cpython llvm llvm_jit)

# RUN(NAME symbolics_01        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
# RUN(NAME symbolics_02        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
RUN(NAME symbolics_03        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
RUN(NAME symbolics_04        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
# RUN(NAME symbolics_05        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
# RUN(NAME symbolics_06        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
# RUN(NAME symbolics_07        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
RUN(NAME symbolics_08        LABELS cpython_sym llvm_sym llvm_jit EXTRA_ARGS --enable-symengine) # renable c_sym
# RUN(NAME symbolics_09        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
RUN(NAME symbolics_10        LABELS cpython_sym c_sym llvm_sym NOFAST EXTRA_ARGS --enable-symengine)
# RUN(NAME symbolics_11        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
# RUN(NAME symbolics_12        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
RUN(NAME symbolics_13        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
RUN(NAME symbolics_14        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
RUN(NAME test_gruntz         LABELS cpython_sym c_sym llvm_sym NOFAST EXTRA_ARGS --enable-symengine)
# RUN(NAME symbolics_15        LABELS c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
RUN(NAME symbolics_16        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
# RUN(NAME symbolics_17        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
RUN(NAME symbolics_18        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)
# RUN(NAME gruntz_demo3        LABELS cpython_sym c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)

# RUN(NAME sizeof_01           LABELS llvm c
#         EXTRAFILES sizeof_01b.c)
RUN(NAME sizeof_02           LABELS cpython llvm llvm_jit c)
# RUN(NAME enum_01             LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME enum_02             LABELS cpython llvm llvm_jit NOFAST)
RUN(NAME enum_03             LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME enum_04             LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME enum_05             LABELS llvm c
        EXTRAFILES enum_05b.c NOFAST)
# RUN(NAME enum_06             LABELS cpython llvm llvm_jit c)
RUN(NAME enum_07             IMPORT_PATH ..
                             LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME union_01            LABELS cpython llvm llvm_jit c)
# RUN(NAME union_02            LABELS cpython llvm llvm_jit c NOFAST)
RUN(NAME union_03            LABELS cpython llvm llvm_jit c)
RUN(NAME union_04            IMPORT_PATH ..
                             LABELS cpython llvm llvm_jit c)
# RUN(NAME test_str_to_int     LABELS cpython llvm llvm_jit c)
# RUN(NAME test_platform       LABELS cpython llvm llvm_jit) # renable c
RUN(NAME test_vars_01        LABELS cpython llvm llvm_jit)
RUN(NAME test_version        LABELS cpython llvm llvm_jit) 
RUN(NAME logical_binop1      LABELS cpython llvm llvm_jit)
# RUN(NAME test_logical_compare           LABELS cpython llvm llvm_jit) # TODO: Add C backend after fixing issue #2708
# RUN(NAME test_logical_assignment        LABELS cpython llvm llvm_jit) # TODO: Add C backend after fixing issue #2708
RUN(NAME vec_01              LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME test_str_comparison LABELS cpython llvm llvm_jit c wasm)
RUN(NAME test_bit_length     LABELS cpython) # renable c, FIXME: This test fails on llvm & llvm_jit
# RUN(NAME str_to_list_cast    LABELS cpython llvm llvm_jit c)
# RUN(NAME cast_01             LABELS cpython llvm llvm_jit c)
# RUN(NAME cast_02             LABELS cpython llvm llvm_jit c)
# RUN(NAME test_sys_01         LABELS cpython llvm llvm_jit c NOFAST)
# RUN(NAME intent_01           LABELS cpython llvm llvm_jit)


# RUN(NAME test_package_01     LABELS cpython llvm llvm_jit NOFAST)
# RUN(NAME test_pkg_lpdraw     LABELS cpython llvm llvm_jit wasm)
# RUN(NAME test_pkg_lnn_01        LABELS cpython llvm llvm_jit NOFAST)
# RUN(NAME test_pkg_lnn_02        LABELS cpython llvm llvm_jit NOFAST)
# RUN(NAME test_pkg_lpconvexhull  LABELS cpython llvm llvm_jit c NOFAST)

RUN(NAME generics_01         LABELS cpython llvm llvm_jit) # renable c
# RUN(NAME generics_02         LABELS cpython llvm llvm_jit c)
RUN(NAME generics_array_01   LABELS cpython llvm llvm_jit c)
RUN(NAME generics_array_02   LABELS cpython llvm llvm_jit c)
RUN(NAME generics_array_03   LABELS cpython llvm llvm_jit c)
RUN(NAME generics_list_01    LABELS cpython llvm llvm_jit) # renable c 
RUN(NAME test_statistics_01  LABELS cpython llvm llvm_jit NOFAST)
# RUN(NAME test_statistics_02  LABELS cpython llvm llvm_jit NOFAST REQ_PY_VER 3.10)
# RUN(NAME test_attributes     LABELS cpython llvm llvm_jit)
# RUN(NAME test_str_attributes LABELS cpython llvm llvm_jit c)
# RUN(NAME kwargs_01           LABELS cpython llvm llvm_jit NOFAST) # renable c
RUN(NAME def_func_01         LABELS cpython llvm llvm_jit) # renable c 

RUN(NAME func_inline_01 LABELS llvm llvm_jit c wasm)
RUN(NAME func_inline_02 LABELS cpython llvm llvm_jit c)
RUN(NAME func_static_01 LABELS cpython llvm llvm_jit c wasm)
RUN(NAME func_static_02 LABELS cpython llvm llvm_jit c wasm)
RUN(NAME func_dep_03    LABELS cpython llvm llvm_jit c)
RUN(NAME func_dep_04    LABELS cpython llvm llvm_jit c)
RUN(NAME func_internal_def_01 LABELS cpython llvm llvm_jit NOFAST)
RUN(NAME func_01        LABELS cpython llvm llvm_jit)
RUN(NAME func_02        LABELS c_sym llvm_sym llvm_jit NOFAST EXTRA_ARGS --enable-symengine)

RUN(NAME float_01 LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME recursive_01 LABELS cpython llvm llvm_jit c wasm wasm_x64 wasm_x86)
RUN(NAME comp_01 LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME bit_operations_i32 LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME bit_operations_i64 LABELS cpython llvm llvm_jit c wasm)

# RUN(NAME test_argv_01      LABELS cpython llvm NOFAST)
RUN(NAME global_syms_01    LABELS cpython llvm llvm_jit c)
RUN(NAME global_syms_02    LABELS cpython llvm llvm_jit c)
# RUN(NAME global_syms_03_b  LABELS cpython llvm llvm_jit c)
# RUN(NAME global_syms_03_c  LABELS cpython llvm llvm_jit c)
RUN(NAME global_syms_04    LABELS cpython llvm llvm_jit c wasm wasm_x64)
RUN(NAME global_syms_05    LABELS cpython llvm llvm_jit c) 
RUN(NAME global_syms_06    LABELS cpython llvm llvm_jit c)

RUN(NAME callback_01       LABELS cpython llvm llvm_jit c)
RUN(NAME callback_02       LABELS cpython llvm llvm_jit c)
RUN(NAME callback_03       LABELS cpython llvm llvm_jit c)

RUN(NAME lambda_01         LABELS cpython llvm llvm_jit)

RUN(NAME c_mangling        LABELS cpython llvm llvm_jit) # renable c 
# RUN(NAME class_01          LABELS cpython llvm llvm_jit)
# RUN(NAME class_02          LABELS cpython llvm llvm_jit)
# RUN(NAME class_03          LABELS cpython llvm llvm_jit)
# RUN(NAME class_04          LABELS cpython llvm llvm_jit)
# RUN(NAME class_05          LABELS cpython llvm llvm_jit)
# RUN(NAME class_06          LABELS cpython llvm llvm_jit)


# callback_04 is to test emulation. So just run with cpython
RUN(NAME callback_04       IMPORT_PATH .. LABELS cpython)

# Intrinsic Functions
RUN(NAME intrinsics_01     LABELS cpython llvm llvm_jit NOFAST) # any
RUN(NAME intrinsics_02     LABELS cpython llvm llvm_jit c) # floordiv
# RUN(NAME test_builtin_type LABELS cpython llvm llvm_jit c) # type
# RUN(NAME test_builtin_type_set LABELS cpython llvm llvm_jit) # type (specifically for `set`)

# lpython decorator
# RUN(NAME lpython_decorator_01    LABELS cpython)
# RUN(NAME lpython_decorator_02    LABELS cpython)

COMPILE(NAME import_order_01     LABELS cpython llvm llvm_jit c) # any

# LPython emulation mode
RUN(NAME lpython_emulation_01    LABELS cpython NOMOD)
