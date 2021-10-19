cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(lfortran_runtime_builtin C Fortran)

add_library(lfortran_runtime_builtin
    STATIC
    lfortran_intrinsic_builtin.f90
)
