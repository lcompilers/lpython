find_path(LINK_INCLUDE_DIR link.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LINKH DEFAULT_MSG LINK_INCLUDE_DIR)

add_library(p::link INTERFACE IMPORTED)
set_property(TARGET p::link PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${LINK_INCLUDE_DIR})
