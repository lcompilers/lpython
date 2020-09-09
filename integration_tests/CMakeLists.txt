cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(lfort C Fortran)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug
        CACHE STRING "Build type (Debug, Release)" FORCE)
endif ()
if (NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" OR
        CMAKE_BUILD_TYPE STREQUAL "Release"))
    message("${CMAKE_BUILD_TYPE}")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be one of: Debug, Release (current value: '${CMAKE_BUILD_TYPE}')")
endif ()

set(LFORTRAN_BACKEND no CACHE STRING "Only compile the LFortran subset for the given backend")

enable_testing()

message("\n")
message("Configuration results")
message("---------------------")
message("Fortran compiler: ${CMAKE_Fortran_COMPILER}")
message("C compiler      : ${CMAKE_C_COMPILER}")
message("Build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Fortran compiler flags: ${CMAKE_Fortran_FLAGS_DEBUG}")
    message("C compiler flags      : ${CMAKE_C_FLAGS_DEBUG}")
else ()
    message("Fortran compiler flags: ${CMAKE_Fortran_FLAGS_RELEASE}")
    message("C compiler flags      : ${CMAKE_C_FLAGS_RELEASE}")
endif ()
message("Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message("LFORTRAN_BACKEND: ${LFORTRAN_BACKEND}")



macro(RUN)
    set(options FAIL)
    set(oneValueArgs NAME)
    set(multiValueArgs LABELS)
    cmake_parse_arguments(RUN "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )
    set(name ${RUN_NAME})
    if (NOT name)
        message(FATAL_ERROR "Must specify the NAME argument")
    endif()
    if (LFORTRAN_BACKEND)
        if (${LFORTRAN_BACKEND} IN_LIST RUN_LABELS)
            # Test is supported by the given LFortran backend
            set(ADD_TEST ON)
        else()
            # Test is not supported by the given LFortran backend
            set(ADD_TEST OFF)
        endif()
    else()
        # GFortran
        set(ADD_TEST ON)
    endif()

    if (ADD_TEST)
        add_executable(${name} ${name}.f90)
        if ((LFORTRAN_BACKEND STREQUAL "cpp") OR (LFORTRAN_BACKEND STREQUAL "x86"))
            target_compile_options(${name} PUBLIC --backend=${LFORTRAN_BACKEND})
            target_link_options(${name} PUBLIC --backend=${LFORTRAN_BACKEND})
        endif()
        add_test(${name} ${CURRENT_BINARY_DIR}/${name})

        if (RUN_LABELS)
            set_tests_properties(${name} PROPERTIES LABELS "${RUN_LABELS}")
        endif()

        if (${RUN_FAIL})
            set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
        endif()
    endif()
endmacro(RUN)


# GFortran + LFortran LLVM + LFortran C++

RUN(NAME program_cmake_01 LABELS gfortran llvm cpp x86)
RUN(NAME program_cmake_02 LABELS gfortran llvm cpp x86)

RUN(NAME abort_01 FAIL LABELS gfortran llvm cpp x86)

RUN(NAME print_01 LABELS gfortran llvm cpp)

RUN(NAME cond_01 LABELS gfortran llvm cpp x86)

RUN(NAME expr_01 FAIL LABELS gfortran llvm cpp x86)
RUN(NAME expr_02 LABELS gfortran llvm cpp x86)
RUN(NAME expr_03 LABELS gfortran llvm cpp x86)
RUN(NAME expr_04 LABELS gfortran llvm cpp)

RUN(NAME variables_01 LABELS gfortran llvm cpp x86)
RUN(NAME variables_02 LABELS gfortran llvm cpp x86)
RUN(NAME variables_03 LABELS gfortran llvm cpp)

RUN(NAME if_01 LABELS gfortran llvm cpp x86)
RUN(NAME if_02 FAIL LABELS gfortran llvm cpp x86)
RUN(NAME if_03 FAIL LABELS gfortran llvm cpp x86)

RUN(NAME while_01 LABELS gfortran llvm cpp x86)
RUN(NAME while_02 LABELS gfortran llvm cpp)

RUN(NAME doloop_01 LABELS gfortran llvm cpp x86)
RUN(NAME doloop_02 LABELS gfortran llvm cpp x86)
RUN(NAME doloop_03 LABELS gfortran llvm cpp)

RUN(NAME subroutines_01 LABELS gfortran llvm cpp x86)
RUN(NAME subroutines_02 LABELS gfortran llvm cpp x86)

RUN(NAME functions_01 LABELS gfortran llvm cpp x86)

RUN(NAME types_01 LABELS gfortran llvm cpp)
RUN(NAME types_02 LABELS gfortran llvm cpp)
RUN(NAME types_03 LABELS gfortran llvm cpp)
RUN(NAME types_04 LABELS gfortran llvm cpp)
RUN(NAME types_05 LABELS gfortran llvm cpp)
RUN(NAME types_06 LABELS gfortran llvm cpp)

# GFortran + LFortran C++
RUN(NAME doconcurrentloop_01 LABELS gfortran cpp)

RUN(NAME arrays_01 LABELS gfortran cpp)
RUN(NAME arrays_03 LABELS gfortran cpp)
RUN(NAME arrays_04 LABELS gfortran cpp)
RUN(NAME arrays_05 LABELS gfortran cpp)

# GFortran
RUN(NAME arrays_02 LABELS gfortran)
RUN(NAME arrays_06 LABELS gfortran)
RUN(NAME arrays_07 LABELS gfortran)

RUN(NAME intrinsics_01 LABELS gfortran)

RUN(NAME modules_01 LABELS gfortran)
RUN(NAME modules_02 LABELS gfortran)
RUN(NAME modules_03 LABELS gfortran)
RUN(NAME modules_04 LABELS gfortran)
RUN(NAME modules_05 LABELS gfortran)

RUN(NAME if_04 LABELS gfortran x86)
RUN(NAME case_01 LABELS gfortran)

RUN(NAME program_02 LABELS gfortran)

RUN(NAME where_01 LABELS gfortran)

RUN(NAME forallloop_01 LABELS gfortran)

RUN(NAME subroutines_03 LABELS gfortran)

RUN(NAME interface_01 LABELS gfortran)
RUN(NAME interface_02 LABELS gfortran)
RUN(NAME interface_03 LABELS gfortran)

RUN(NAME types_07 LABELS gfortran)
RUN(NAME types_08 LABELS gfortran)
RUN(NAME types_09 LABELS gfortran)
RUN(NAME types_10 LABELS gfortran)
RUN(NAME types_11 LABELS gfortran)
RUN(NAME types_12 LABELS gfortran)
RUN(NAME types_13 LABELS gfortran)

RUN(NAME doloop_04 LABELS gfortran)

RUN(NAME derived_types_01 LABELS gfortran)
RUN(NAME derived_types_02 LABELS gfortran)
RUN(NAME derived_types_03 LABELS gfortran)

RUN(NAME line_continuation_01 LABELS gfortran)

RUN(NAME program_01 LABELS gfortran)
RUN(NAME program_03 LABELS gfortran)

RUN(NAME allocate_01 LABELS gfortran)
RUN(NAME block_01 LABELS gfortran)
RUN(NAME associate_01 LABELS gfortran)
