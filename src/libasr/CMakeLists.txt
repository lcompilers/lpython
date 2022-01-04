cmake_minimum_required(VERSION 3.15)

project(libasr)

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17
        CACHE STRING "C++ standard" FORCE)
endif ()

if (NOT LFORTRAN_VERSION)
    set(LFORTRAN_VERSION "0.1-git"
        CACHE STRING "LFortran version" FORCE)
endif ()

configure_file(config.h.in config.h)

set(SRC
    codegen/fortran_evaluator.cpp
    codegen/asr_to_cpp.cpp
    codegen/asr_to_py.cpp
    codegen/x86_assembler.cpp
    codegen/asr_to_x86.cpp

    pass/param_to_const.cpp
    pass/do_loops.cpp
    pass/for_all.cpp
    pass/global_stmts.cpp
    pass/select_case.cpp
    pass/implied_do_loops.cpp
    pass/array_op.cpp
    pass/class_constructor.cpp
    pass/arr_slice.cpp
    pass/print_arr.cpp
    pass/pass_utils.cpp
    pass/unused_functions.cpp

    asr_verify.cpp
    asr_utils.cpp
    diagnostics.cpp
    stacktrace.cpp
    string_utils.cpp
    asr_scopes.cpp
    modfile.cpp
    serialization.cpp
)
if (WITH_LLVM)
    set(SRC ${SRC}
        codegen/evaluator.cpp
        codegen/asr_to_llvm.cpp
        codegen/llvm_array_utils.cpp
        codegen/llvm_utils.cpp
        pass/nested_vars.cpp
    )
    # We use deprecated API in LLVM, so we disable the warning until we upgrade
    if (NOT MSVC)
        set_source_files_properties(codegen/evaluator.cpp PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
        set_source_files_properties(codegen/asr_to_llvm.cpp PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
        set_source_files_properties(codegen/llvm_array_utils.cpp PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
        set_source_files_properties(codegen/llvm_utils.cpp PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
    endif()
endif()
add_library(asr ${SRC})
target_include_directories(asr BEFORE PUBLIC ${libasr_SOURCE_DIR}/..)
target_include_directories(asr BEFORE PUBLIC ${libasr_BINARY_DIR}/..)
if (WITH_BFD)
    target_link_libraries(lfortran_lib p::bfd)
endif()
if (WITH_LINK)
    target_link_libraries(lfortran_lib p::link)
endif()
if (WITH_EXECINFO)
    target_link_libraries(lfortran_lib p::execinfo)
endif()
if (WITH_LLVM)
    target_link_libraries(lfortran_lib p::llvm)
endif()
