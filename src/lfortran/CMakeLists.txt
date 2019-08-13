set(SRC
    parser/tokenizer.cpp
    parser/parser.tab.cc
    parser/parser.cpp
)
add_library(lfortran_lib ${SRC})
target_include_directories(lfortran_lib BEFORE PUBLIC ${lfortran_SOURCE_DIR}/src)
install(TARGETS lfortran_lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
