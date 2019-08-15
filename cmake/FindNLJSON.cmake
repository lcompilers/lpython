find_path(NLJSON_HEADER nlohmann/json.hpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NLJSON DEFAULT_MSG NLJSON_HEADER)

add_library(p::nljson INTERFACE IMPORTED)
set_property(TARGET p::nljson PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${NLJSON_HEADER})
