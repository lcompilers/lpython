find_path(MACHO_INCLUDE_DIR mach-o/dyld.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MACHO DEFAULT_MSG MACHO_INCLUDE_DIR)

add_library(p::macho INTERFACE IMPORTED)
set_property(TARGET p::macho PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${MACHO_INCLUDE_DIR})
