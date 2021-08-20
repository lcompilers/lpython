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
    set(multiValueArgs LABELS EXTRAFILES)
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
        add_executable(${name} ${name}.f90 ${RUN_EXTRAFILES})
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
RUN(NAME expr_05 LABELS gfortran llvm cpp)
RUN(NAME expr_06 LABELS gfortran llvm)
RUN(NAME expr_07 LABELS gfortran llvm)

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
RUN(NAME doloop_04 LABELS gfortran)
RUN(NAME doloop_05 LABELS gfortran llvm cpp)

RUN(NAME goto_01 LABELS gfortran)
RUN(NAME goto_02 LABELS gfortran)
RUN(NAME goto_03 LABELS gfortran)

RUN(NAME subroutines_01 LABELS gfortran llvm cpp x86)
RUN(NAME subroutines_02 LABELS gfortran llvm cpp x86)
RUN(NAME subroutines_04 LABELS gfortran llvm)

RUN(NAME functions_01 LABELS gfortran llvm cpp x86)
RUN(NAME functions_02 LABELS gfortran llvm)
RUN(NAME functions_03 LABELS gfortran llvm)

RUN(NAME types_01 LABELS gfortran llvm cpp)
RUN(NAME types_02 LABELS gfortran llvm cpp)
RUN(NAME types_03 LABELS gfortran llvm cpp)
RUN(NAME types_04 LABELS gfortran llvm cpp)
RUN(NAME types_05 LABELS gfortran llvm cpp)
RUN(NAME types_06 LABELS gfortran llvm cpp)

# GFortran + LFortran C++
RUN(NAME doconcurrentloop_01 LABELS gfortran cpp)

RUN(NAME arrays_01 LABELS gfortran cpp llvm)
RUN(NAME arrays_01_size LABELS gfortran llvm)
RUN(NAME arrays_01_real LABELS gfortran llvm)
RUN(NAME arrays_01_complex LABELS gfortran llvm)
RUN(NAME arrays_01_logical LABELS gfortran llvm)
RUN(NAME array_bound_1 LABELS gfortran llvm)
RUN(NAME arrays_op_1 LABELS gfortran llvm)
RUN(NAME arrays_op_2 LABELS gfortran llvm)
RUN(NAME arrays_op_3 LABELS gfortran llvm)
RUN(NAME arrays_op_4 LABELS gfortran llvm)
RUN(NAME arrays_op_5 LABELS gfortran llvm)
RUN(NAME arrays_op_6 LABELS gfortran llvm)
RUN(NAME arrays_op_7 LABELS gfortran llvm)
RUN(NAME arrays_03_func LABELS gfortran cpp llvm)
RUN(NAME arrays_04_func LABELS gfortran cpp llvm)
RUN(NAME arrays_05 LABELS gfortran cpp)

# GFortran
RUN(NAME arrays_02 LABELS gfortran)
RUN(NAME arrays_06 LABELS gfortran llvm)
RUN(NAME arrays_07 LABELS gfortran llvm)
RUN(NAME arrays_08_func LABELS gfortran llvm)
RUN(NAME arrays_09 LABELS gfortran)
RUN(NAME arrays_10 LABELS gfortran)

RUN(NAME reserved_01 LABELS gfortran)
RUN(NAME reserved_02 LABELS gfortran llvm)
RUN(NAME reserved_03 LABELS gfortran)

RUN(NAME namelist_01 LABELS gfortran)

RUN(NAME format_01 LABELS gfortran)
RUN(NAME format_02 LABELS gfortran)

RUN(NAME submodule_01 LABELS gfortran)

RUN(NAME floor_01 LABELS gfortran llvm) # floor body
RUN(NAME floor_02 LABELS gfortran) # floor symboltable

RUN(NAME int_01 LABELS gfortran llvm) # int body
RUN(NAME int_02 LABELS gfortran) # int symboltable

RUN(NAME intrinsics_01 LABELS gfortran) # sqrt, abs, log
RUN(NAME intrinsics_02 LABELS gfortran llvm) # sin
RUN(NAME intrinsics_03 LABELS gfortran llvm) # cos
RUN(NAME intrinsics_04 LABELS gfortran llvm) # tan
RUN(NAME intrinsics_05 LABELS gfortran llvm) # hyperbolics
RUN(NAME intrinsics_06 LABELS gfortran llvm) # inverse trignometric
RUN(NAME intrinsics_07 LABELS gfortran) # kind
RUN(NAME intrinsics_08 LABELS gfortran) # tiny symboltable
RUN(NAME intrinsics_09 LABELS gfortran) # tiny body
RUN(NAME intrinsics_10 LABELS gfortran llvm) # real body
RUN(NAME intrinsics_11 LABELS gfortran) # real symboltable
RUN(NAME intrinsics_12 LABELS gfortran llvm) # kind body
RUN(NAME intrinsics_13 LABELS gfortran) # kind symboltable
RUN(NAME intrinsics_14 LABELS gfortran) # selected_real,int_kind body
RUN(NAME intrinsics_15 LABELS gfortran llvm) # real
RUN(NAME intrinsics_16 LABELS gfortran) # aimag
RUN(NAME intrinsics_open_close_read_write LABELS gfortran)

RUN(NAME parameter_01 LABELS gfortran)
RUN(NAME parameter_02 LABELS gfortran)
RUN(NAME parameter_03 LABELS gfortran llvm)
RUN(NAME parameter_04 LABELS gfortran llvm) # selected_real,int_kind symboltable
RUN(NAME parameter_05 LABELS gfortran llvm) # Implicit IntegerToReal

RUN(NAME modules_01 LABELS gfortran llvm)
RUN(NAME modules_02 LABELS gfortran llvm)
RUN(NAME modules_03 LABELS gfortran)
RUN(NAME modules_04 LABELS gfortran)
RUN(NAME modules_05 LABELS gfortran)
RUN(NAME modules_06 LABELS gfortran llvm)
RUN(NAME modules_07 LABELS gfortran llvm EXTRAFILES modules_07_module.f90)
RUN(NAME modules_08 LABELS gfortran llvm EXTRAFILES
        modules_08_a.f90 modules_08_b.f90)
RUN(NAME modules_09 LABELS gfortran llvm EXTRAFILES
        modules_09_a.f90 modules_09_b.f90)
RUN(NAME modules_10 LABELS gfortran llvm)
RUN(NAME modules_11 LABELS gfortran llvm)
RUN(NAME modules_12 LABELS gfortran)
RUN(NAME modules_13 LABELS gfortran llvm)
RUN(NAME modules_14 LABELS gfortran llvm EXTRAFILES modules_14_a.f90)
RUN(NAME modules_15 LABELS gfortran llvm EXTRAFILES
        modules_15b.f90 modules_15c.c)

RUN(NAME if_04 LABELS gfortran x86)
RUN(NAME case_01 LABELS gfortran llvm)
RUN(NAME case_02 LABELS gfortran llvm)
RUN(NAME case_03 LABELS gfortran llvm)
RUN(NAME case_04 LABELS gfortran)

RUN(NAME select_type_01 LABELS gfortran)

RUN(NAME program_02 LABELS gfortran)
RUN(NAME program_03 LABELS gfortran llvm)
RUN(NAME program_04 LABELS gfortran)

RUN(NAME where_01 LABELS gfortran)

RUN(NAME forallloop_01 LABELS gfortran)

RUN(NAME subroutines_03 LABELS gfortran)

RUN(NAME parsing_01 LABELS gfortran)
RUN(NAME parsing_02 LABELS gfortran)
RUN(NAME parsing_03 LABELS gfortran llvm)

RUN(NAME interface_01 LABELS gfortran llvm)
RUN(NAME interface_02 LABELS gfortran llvm)
RUN(NAME interface_03 LABELS gfortran)
RUN(NAME interface_04 LABELS gfortran llvm)
RUN(NAME interface_05 LABELS gfortran llvm)
RUN(NAME interface_06 LABELS gfortran)
RUN(NAME interface_07 LABELS gfortran llvm)

RUN(NAME types_07 LABELS gfortran)
RUN(NAME types_08 LABELS gfortran)
RUN(NAME types_09 LABELS gfortran)
RUN(NAME types_10 LABELS gfortran)
RUN(NAME types_11 LABELS gfortran)
RUN(NAME types_12 LABELS gfortran llvm)
RUN(NAME types_13 LABELS gfortran)
RUN(NAME types_14 LABELS gfortran llvm)

RUN(NAME complex_01 LABELS gfortran)
RUN(NAME complex_02 LABELS gfortran)
RUN(NAME complex_sub_test LABELS gfortran llvm)
RUN(NAME complex_mul_test LABELS gfortran llvm)
RUN(NAME complex_div_test LABELS gfortran llvm)
RUN(NAME complex_pow_test LABELS gfortran llvm)

RUN(NAME logical1 LABELS gfortran llvm)
RUN(NAME logical2 LABELS gfortran llvm)
RUN(NAME logical3 LABELS gfortran llvm)
RUN(NAME logical4 LABELS gfortran llvm)

# `reduce` is not supported by GFortran yet:
# RUN(NAME doconcurrentloop_02 LABELS gfortran)

RUN(NAME derived_types_01 LABELS gfortran llvm)
RUN(NAME derived_types_02 LABELS gfortran)
RUN(NAME derived_types_03 LABELS gfortran)

RUN(NAME line_continuation_01 LABELS gfortran llvm)
RUN(NAME line_continuation_02 LABELS gfortran llvm)
RUN(NAME line_continuation_03 LABELS gfortran llvm)

RUN(NAME program_01 LABELS gfortran)
RUN(NAME init_values LABELS gfortran llvm)
RUN(NAME param_pass_01 LABELS gfortran)

RUN(NAME allocate_01 LABELS gfortran llvm)
RUN(NAME allocate_02 LABELS gfortran llvm)
RUN(NAME allocate_03 LABELS gfortran llvm)
RUN(NAME block_01 LABELS gfortran)
RUN(NAME block_02 LABELS gfortran)
RUN(NAME associate_01 LABELS gfortran)
RUN(NAME associate_02 LABELS gfortran llvm)
RUN(NAME associate_03 LABELS gfortran llvm)
RUN(NAME associate_04 LABELS gfortran llvm)
RUN(NAME associate_05 LABELS gfortran llvm)

RUN(NAME real_dp LABELS gfortran llvm)
RUN(NAME bin_op_real_dp LABELS gfortran llvm)
RUN(NAME const_real_dp LABELS gfortran llvm)
RUN(NAME real_dp_param LABELS gfortran llvm)
RUN(NAME int_dp LABELS gfortran llvm)
RUN(NAME int_dp_param LABELS gfortran llvm)
RUN(NAME complex_dp LABELS gfortran llvm)
RUN(NAME bin_op_complex_dp LABELS gfortran llvm)
RUN(NAME complex_dp_param LABELS gfortran llvm)
RUN(NAME const_kind_01 LABELS gfortran llvm)
RUN(NAME const_kind_02 LABELS gfortran llvm)

RUN(NAME string1 LABELS gfortran llvm cpp)
RUN(NAME string2 LABELS gfortran llvm)
RUN(NAME string3 LABELS gfortran llvm)
RUN(NAME string4 LABELS gfortran llvm)

RUN(NAME nested_01 LABELS gfortran llvm)
RUN(NAME nested_02 LABELS gfortran llvm)
RUN(NAME nested_03 LABELS gfortran llvm)
RUN(NAME nested_04 LABELS gfortran llvm)
RUN(NAME nested_05 LABELS gfortran llvm)
RUN(NAME nested_06 LABELS gfortran llvm)

RUN(NAME intent_01 LABELS gfortran llvm)

RUN(NAME callback_01 LABELS gfortran llvm)
RUN(NAME callback_02 LABELS gfortran llvm)
RUN(NAME callback_03 LABELS gfortran llvm)
RUN(NAME callback_04 LABELS gfortran llvm)
RUN(NAME callback_05 LABELS gfortran llvm)

RUN(NAME recursion_01 LABELS gfortran llvm)
RUN(NAME recursion_02 LABELS gfortran llvm)
RUN(NAME recursion_03 LABELS gfortran llvm)

RUN(NAME return_01 LABELS gfortran llvm)
RUN(NAME return_02 LABELS gfortran llvm)
RUN(NAME return_03 LABELS gfortran llvm)

RUN(NAME class_01 LABELS gfortran llvm)
RUN(NAME class_02 LABELS gfortran llvm)
RUN(NAME class_03 LABELS gfortran llvm)
RUN(NAME class_04 LABELS gfortran llvm)

RUN(NAME test_iso_c_binding LABELS gfortran llvm)
RUN(NAME test_iso_fortran_env LABELS gfortran llvm)

RUN(NAME abs_01 LABELS gfortran llvm)
RUN(NAME abs_02 LABELS gfortran llvm)
RUN(NAME sqrt_01 LABELS gfortran llvm)
RUN(NAME sqrt_02 LABELS gfortran llvm)
RUN(NAME sin_01 LABELS gfortran llvm)
RUN(NAME sin_02 LABELS gfortran llvm)
RUN(NAME sin_03 LABELS gfortran llvm)
