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

set(LFORTRAN_SUBSET no CACHE STRING "Only compile the LFortran subset")

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
message("LFORTRAN_SUBSET: ${LFORTRAN_SUBSET}")



macro(RUN name)
    add_executable(${name} ${name}.f90)
    add_test(${name} ${CURRENT_BINARY_DIR}/${name})
    set(extra_args ${ARGN})
    if (extra_args STREQUAL "fail")
        set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
    endif()
endmacro(RUN)

macro(COMPILE name)
    add_executable(${name} ${name}.f90)
endmacro(COMPILE)


RUN(program_cmake_01)

RUN(abort_01 fail)

RUN(cond_01)

RUN(expr_01 fail)
RUN(expr_02)
RUN(expr_03)

RUN(variables_01)
RUN(variables_02)
RUN(variables_03)

RUN(if_01)
RUN(if_02 fail)
RUN(if_03 fail)

RUN(print_01)

RUN(while_01)

if (NOT LFORTRAN_SUBSET)
    RUN(program_cmake_02)

    RUN(while_02)
    RUN(doloop_01)
    RUN(doloop_02)
    RUN(doloop_03)

    RUN(arrays_01)
    RUN(arrays_02)

    RUN(subroutines_01)

    RUN(intrinsics_01)
endif()
