cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(project1 C Fortran)

add_executable(project1
    project1.f90
    a.f90
    b.f90
)
