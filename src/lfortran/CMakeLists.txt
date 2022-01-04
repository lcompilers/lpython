configure_file(../libasr/config.h.in ../libasr/config.h)

set(SRC
    parser/preprocessor.cpp
    parser/tokenizer.cpp
    parser/parser.tab.cc
    parser/parser.cpp

    semantics/ast_symboltable_visitor.cpp
    semantics/ast_body_visitor.cpp
    semantics/ast_to_asr.cpp
    ../libasr/asr_scopes.cpp

    ../libasr/codegen/fortran_evaluator.cpp
    ../libasr/codegen/asr_to_cpp.cpp
    ../libasr/codegen/asr_to_py.cpp
    ../libasr/codegen/x86_assembler.cpp
    ../libasr/codegen/asr_to_x86.cpp

    ../libasr/pass/param_to_const.cpp
    ../libasr/pass/do_loops.cpp
    ../libasr/pass/for_all.cpp
    ../libasr/pass/global_stmts.cpp
    ../libasr/pass/select_case.cpp
    ../libasr/pass/implied_do_loops.cpp
    ../libasr/pass/array_op.cpp
    ../libasr/pass/class_constructor.cpp
    ../libasr/pass/arr_slice.cpp
    ../libasr/pass/print_arr.cpp
    ../libasr/pass/pass_utils.cpp
    ../libasr/pass/unused_functions.cpp

    ../libasr/asr_verify.cpp
    ../libasr/asr_utils.cpp
    ../libasr/modfile.cpp
    pickle.cpp
    ast_serialization.cpp
    ../libasr/serialization.cpp
    cwrapper.cpp

    ../libasr/diagnostics.cpp

    ast_to_src.cpp
    ast_to_openmp.cpp

    mod_to_asr.cpp

    ../libasr/string_utils.cpp
    utils.cpp

    ../libasr/stacktrace.cpp

    ../bin/tpl/whereami/whereami.cpp
)
if (WITH_XEUS)
    set(SRC ${SRC}
        fortran_kernel.cpp
    )
endif()
if (WITH_JSON)
    set(SRC ${SRC}
        ast_to_json.cpp
    )
endif()
if (WITH_LLVM)
    set(SRC ${SRC}
        ../libasr/codegen/evaluator.cpp
        ../libasr/codegen/asr_to_llvm.cpp
        ../libasr/codegen/llvm_array_utils.cpp
        ../libasr/codegen/llvm_utils.cpp
        ../libasr/pass/nested_vars.cpp
    )
    # We use deprecated API in LLVM, so we disable the warning until we upgrade
    if (NOT MSVC)
        set_source_files_properties(../libasr/codegen/evaluator.cpp PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
        set_source_files_properties(../libasr/codegen/asr_to_llvm.cpp PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
        set_source_files_properties(../libasr/codegen/llvm_array_utils.cpp PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
        set_source_files_properties(../libasr/codegen/llvm_utils.cpp PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
    endif()
endif()
add_library(lfortran_lib ${SRC})
target_link_libraries(lfortran_lib lfortran_runtime_static ZLIB::ZLIB)
target_include_directories(lfortran_lib BEFORE PUBLIC ${lfortran_SOURCE_DIR}/src)
target_include_directories(lfortran_lib BEFORE PUBLIC ${lfortran_BINARY_DIR}/src)
if (WITH_XEUS)
    target_link_libraries(lfortran_lib xeus)
endif()
if (WITH_JSON)
    target_link_libraries(lfortran_lib p::rapidjson)
endif()
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
#install(TARGETS lfortran_lib
#    RUNTIME DESTINATION bin
#    ARCHIVE DESTINATION lib
#    LIBRARY DESTINATION lib
#)

add_subdirectory(tests)
