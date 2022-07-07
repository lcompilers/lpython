project(lsp)
add_library(lsp ${lsp_SOURCE_DIR})

if (NOT HAVE_LFORTRAN_RAPIDJSON)
    message(FATAL_ERROR "Expected RapidJSON to be found, but couldn't find it in the path.")
endif()
# TODO: Removing all the following commented out code for now, @certik - can you please confirm if none of this is required?
# target_include_directories(lsp PRIVATE "tpl")
# target_link_libraries(lsp lpython_lib)
# find_package(RapidJSON REQUIRED)
# # message(STATUS "Found RapidJSON version: ${RapidJSON_PACKAGE_VERSION}")

# if (lsp_STATIC_BIN)
#     if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
#         # Link statically on Linux with gcc or clang
#         if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
#                 CMAKE_CXX_COMPILER_ID MATCHES Clang)
#             target_link_options(lsp PRIVATE -static)
#         endif()
#     endif()
# endif()

# if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     target_link_options(lsp PRIVATE "LINKER:--export-dynamic")
# endif()

# set_target_properties(lsp PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<0:>)

# set_target_properties(lsp PROPERTIES
#     INSTALL_RPATH_USE_LINK_PATH TRUE
# )

# install(TARGETS lsp
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#     ARCHIVE DESTINATION share/lpython_lib/lib
#     LIBRARY DESTINATION share/lpython_lib/lib
# )

# set_target_properties(lsp PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>
#     LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>
#     ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>)

# target_link_libraries(lsp asr lpython_runtime_static ZLIB::ZLIB)
# target_include_directories(lsp PRIVATE ${lsp_SOURCE_DIR}/src/bin/tpl)
# target_include_directories(lsp BEFORE PUBLIC ${lsp_SOURCE_DIR}/src)
# target_include_directories(lsp BEFORE PUBLIC ${lsp_BINARY_DIR}/src)

# target_link_libraries(lsp p::rapidjson)

if (WITH_XEUS)
    target_link_libraries(lsp xeus)
endif()
if (WITH_BFD)
    target_link_libraries(lsp p::bfd)
endif()
if (WITH_LINK)
    target_link_libraries(lsp p::link)
endif()
if (WITH_EXECINFO)
    target_link_libraries(lsp p::execinfo)
endif()
if (WITH_LLVM)
    target_link_libraries(lsp p::llvm)
endif()
