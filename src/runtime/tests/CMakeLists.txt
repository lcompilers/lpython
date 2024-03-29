cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(lpython_runtime_tests C Fortran)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug
        CACHE STRING "Build type (Debug, Release)" FORCE)
endif ()
if (NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" OR
        CMAKE_BUILD_TYPE STREQUAL "Release"))
    message("${CMAKE_BUILD_TYPE}")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be one of: Debug, Release (current value: '${CMAKE_BUILD_TYPE}')")
endif ()

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

set(SRC
    ../impure/lfortran_intrinsics.c
    ../impure/lfortran_intrinsic_math.f90
    ../impure/lfortran_intrinsic_bit.f90
    ../pure/lfortran_intrinsic_math2.f90
    ../pure/lfortran_intrinsic_string.f90
    ../impure/lfortran_intrinsic_sin.f90
    ../impure/lfortran_intrinsic_sin_c.c
    ../pure/lfortran_intrinsic_kind.f90
    ../builtin/lfortran_intrinsic_builtin.f90
    ../pure/lfortran_intrinsic_iso_fortran_env.f90
    ../pure/lfortran_intrinsic_ieee_arithmetic.f90
    ../pure/lfortran_intrinsic_iso_c_binding.f90
)
add_library(lpython_runtime SHARED ${SRC})
add_library(lpython_runtime_static STATIC ${SRC})

macro(ADDTEST name)
    add_executable(${name} ${name}.f90)
    target_link_libraries(${name} lpython_runtime ${ARGN})
    add_test(${name} ${PROJECT_BINARY_DIR}/${name})
endmacro(ADDTEST)
