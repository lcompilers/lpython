cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(lfortran_runtime_pure C Fortran)

add_library(lfortran_runtime_pure
  SHARED
  lfortran_intrinsic_iso_c_binding.f90
  lfortran_intrinsic_iso_fortran_env.f90
  lfortran_intrinsic_trig.f90
)
