cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(lcompilers_runtime_impure C Fortran)

add_library(lcompilers_runtime_impure
    STATIC
    lfortran_intrinsics.c
    lfortran_intrinsic_sin_c.c
)
