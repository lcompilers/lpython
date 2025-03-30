add_executable(lpython lpython.cpp)
target_include_directories(lpython PRIVATE "tpl")
target_link_libraries(lpython lpython_lib)
if (LPYTHON_STATIC_BIN)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Link statically on Linux with gcc or clang
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
                CMAKE_CXX_COMPILER_ID MATCHES Clang)
            target_link_options(lpython PRIVATE -static)
        endif()
    endif()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(lpython PRIVATE "LINKER:--export-dynamic")
endif()

if (WITH_STACKTRACE AND APPLE AND CMAKE_CXX_COMPILER_ID MATCHES Clang)
    # On macOS we have to call dsymutil to create the dSYM bundle so that the
    # stacktrace can find debugging information corresponding to the lpython
    # binary
    add_custom_command(
        TARGET lpython
        POST_BUILD
        COMMAND dsymutil lpython
    )
    if (WITH_DWARFDUMP)
        add_custom_command(
            TARGET lpython
            POST_BUILD
            COMMAND llvm-dwarfdump --debug-line lpython.dSYM > lpython.dSYM/raw.txt
        )
        add_custom_command(
            TARGET lpython
            POST_BUILD
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../lfortran/src/libasr/dwarf_convert.py lpython.dSYM/raw.txt lpython.dSYM/lines.txt lpython.dSYM/lines.dat
        )
    endif()
endif()


# Ensure "Release" is not appended to the path on Windows:
# https://stackoverflow.com/a/56201564/479532
set_target_properties(lpython PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<0:>)

set_target_properties(lpython PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

if (HAVE_BUILD_TO_WASM)
    # "-g0": Store no debugging information in the generated wasm file. This helps reduce generated file size
    # "-Oz": Optimize for size. With this code size ~ 2.4mb. Without this code size ~49mb
    # "-fexceptions": Enable Cpp exception support
    # "--no-entry": No start function to execute
    # "-s ASSERTIONS": Compile with Assertions which (as per docs) are helpful to debug compilation process
    # "-s ALLOW_MEMORY_GROWTH": Allow dynamic memory growth upto the maximum page size limit
    # "-s WASM_BIGINT": Allow use of i64 integers. ASR is needing this option to be enabled.
    # "-s EXPORTED_RUNTIME_METHODS=['cwrap']": Export cwarp. cwarp helps us to call our EMSCRIPTEN_KEEPALIVE functions
    # "-fsanitize=undefined": Clang's Undefined Behaviour Sanitizer. The LPython parser segfaults.
        # This option is for debugging, but currently helps avoid the segfault in the parser.
    # "-s INITIAL_MEMORY=536870912": Start the wasm linear memory with sufficiently large size 512Mb.

    # Notes:
    # STANDALONE_WASM is disabling support for exceptions, so it is currently omitted
    # In build_to_wasm.sh, we need CMAKE_CXX_FLAGS_DEBUG="-Wall -Wextra -fexceptions" flags for exception support
    set(WASM_COMPILE_FLAGS "-g0 -fexceptions -fsanitize=undefined")
    set(WASM_LINK_FLAGS
      "-g0 -Oz -fexceptions -fsanitize=undefined --preload-file asset_dir -Wall -Wextra --no-entry -sASSERTIONS=1 -s INITIAL_MEMORY=536870912 -s ALLOW_MEMORY_GROWTH=1 -s WASM_BIGINT -s \"EXPORTED_RUNTIME_METHODS=['cwrap']\""
    )
    set_target_properties(lpython PROPERTIES COMPILE_FLAGS ${WASM_COMPILE_FLAGS})
    set_target_properties(lpython PROPERTIES LINK_FLAGS ${WASM_LINK_FLAGS})
endif()

install(TARGETS lpython
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION share/lpython/lib
    LIBRARY DESTINATION share/lpython/lib
)

set_target_properties(lpython PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<0:>)

if (WITH_INTRINSIC_MODULES)
    macro(LPYTHON_COMPILE_MODULE name)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../runtime/${name}.pyc
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/lpython
            ARGS --disable-main -c ${CMAKE_CURRENT_SOURCE_DIR}/../runtime/${name}.py -o ${name}.o
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../runtime
            DEPENDS lpython ${CMAKE_CURRENT_SOURCE_DIR}/../runtime/${name}.py ${ARGN}
            COMMENT "LPython Compiling ${name}.py")
    endmacro(LPYTHON_COMPILE_MODULE)

    LPYTHON_COMPILE_MODULE(lpython_intrinsic_numpy)
    LPYTHON_COMPILE_MODULE(lpython_builtin)

    add_custom_target(lpython_intrinsics
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lpython_intrinsic_numpy.pyc
        ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lpython_builtin.pyc
    )

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lpython_intrinsic_numpy.pyc
            ${CMAKE_CURRENT_BINARY_DIR}/../runtime/lpython_builtin.pyc
        DESTINATION share/lfortran/lib
    )
endif()
