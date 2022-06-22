add_executable(lsp_bin main.cpp JSONRPC2Connection.cpp LPythonServer.cpp)
target_include_directories(lsp_bin PRIVATE "tpl")
target_link_libraries(lsp_bin lpython_lib)
find_package(RapidJSON REQUIRED)

if (lsp_bin_STATIC_BIN)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Link statically on Linux with gcc or clang
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
                CMAKE_CXX_COMPILER_ID MATCHES Clang)
            target_link_options(lsp_bin PRIVATE -static)
        endif()
    endif()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(lsp_bin PRIVATE "LINKER:--export-dynamic")
endif()

set_target_properties(lsp_bin PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<0:>)

set_target_properties(lsp_bin PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(TARGETS lsp_bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION share/lpython_lib/lib
    LIBRARY DESTINATION share/lpython_lib/lib
)

set_target_properties(lsp_bin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>)

target_link_libraries(lsp_bin asr lpython_runtime_static ZLIB::ZLIB)
target_include_directories(lsp_bin PRIVATE ${lsp_bin_SOURCE_DIR}/src/bin/tpl)
target_include_directories(lsp_bin BEFORE PUBLIC ${lsp_bin_SOURCE_DIR}/src)
target_include_directories(lsp_bin BEFORE PUBLIC ${lsp_bin_BINARY_DIR}/src)
target_link_libraries(lsp_bin p::rapidjson)

if (WITH_XEUS)
    target_link_libraries(lsp_bin xeus)
endif()
if (WITH_BFD)
    target_link_libraries(lsp_bin p::bfd)
endif()
if (WITH_LINK)
    target_link_libraries(lsp_bin p::link)
endif()
if (WITH_EXECINFO)
    target_link_libraries(lsp_bin p::execinfo)
endif()
if (WITH_LLVM)
    target_link_libraries(lsp_bin p::llvm)
endif()