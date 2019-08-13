set(SRC
    parse.cpp
)
add_executable(parse ${SRC})
target_link_libraries(parse lfortran_lib)
#install(TARGETS parse
#    RUNTIME DESTINATION bin
#    ARCHIVE DESTINATION lib
#    LIBRARY DESTINATION lib
#)
