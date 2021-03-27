configure_file(config.h.in config.h)

set(SRC
    parser/tokenizer.cpp
    parser/parser.tab.cc
    parser/parser.cpp

    semantics/ast_to_asr.cpp
    semantics/asr_scopes.cpp

    codegen/asr_to_cpp.cpp
    codegen/x86_assembler.cpp
    codegen/asr_to_x86.cpp

    pass/do_loops.cpp
    pass/global_stmts.cpp
    pass/select_case.cpp

    asr_verify.cpp
    pickle.cpp
    serialization.cpp
    cwrapper.cpp

    ast_to_src.cpp
    ast_to_cpp.cpp
    ast_to_cpp_hip.cpp
    ast_to_openmp.cpp

    mod_to_asr.cpp

    string_utils.cpp

    stacktrace.cpp
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
        codegen/evaluator.cpp
        codegen/asr_to_llvm.cpp
    )
endif()
add_library(lfortran_lib ${SRC})
target_link_libraries(lfortran_lib lfortran_runtime_static p::zlib)
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
