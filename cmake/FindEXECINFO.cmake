find_path(EXECINFO_INCLUDE_DIR execinfo.h)

include(FindPackageHandleStandardArgs)
if (CMAKE_SYSTEM_NAME MATCHES "BSD")
    find_library(EXECINFO_LIBRARY execinfo)
    find_package_handle_standard_args(EXECINFO DEFAULT_MSG
        EXECINFO_LIBRARY EXECINFO_INCLUDE_DIR)
else ()
    find_package_handle_standard_args(EXECINFO DEFAULT_MSG
        EXECINFO_INCLUDE_DIR)
endif ()

add_library(p::execinfo INTERFACE IMPORTED)
set_property(TARGET p::execinfo PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${EXECINFO_INCLUDE_DIR})
if (CMAKE_SYSTEM_NAME MATCHES "BSD")
    set_property(TARGET p::execinfo PROPERTY INTERFACE_LINK_LIBRARIES
        ${EXECINFO_LIBRARY})
endif ()
