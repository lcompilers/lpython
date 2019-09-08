configure_file(config.h.in config.h)

set(SRC
    parser/tokenizer.cpp
    parser/parser.tab.cc
    parser/parser.cpp
    pickle.cpp
    cwrapper.cpp
    semantics/ast_to_asr.cpp
)
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
target_include_directories(lfortran_lib BEFORE PUBLIC ${lfortran_SOURCE_DIR}/src)
target_include_directories(lfortran_lib BEFORE PUBLIC ${lfortran_BINARY_DIR}/src)
if (WITH_JSON)
    target_link_libraries(lfortran_lib p::rapidjson)
endif()
if (WITH_LLVM)
    target_link_libraries(lfortran_lib p::llvm)
endif()
install(TARGETS lfortran_lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

add_subdirectory(tests)
