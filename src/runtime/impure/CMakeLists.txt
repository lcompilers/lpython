cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(lfortran_runtime_impure C Fortran)

add_library(lfortran_runtime_impure
    STATIC
    lfortran_intrinsics.c
    lfortran_intrinsic_math.f90
    lfortran_intrinsic_sin.f90
    lfortran_intrinsic_sin_c.c
    lfortran_intrinsic_bit.f90
)
